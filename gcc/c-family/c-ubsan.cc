/* UndefinedBehaviorSanitizer, undefined behavior detector.
   Copyright (C) 2013-2026 Free Software Foundation, Inc.
   Contributed by Marek Polacek <polacek@redhat.com>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "c-family/c-common.h"
#include "ubsan.h"
#include "c-family/c-ubsan.h"
#include "stor-layout.h"
#include "builtins.h"
#include "gimplify.h"
#include "langhooks.h"
#include "stringpool.h"
#include "attribs.h"
#include "asan.h"
#include "cgraph.h"
#include "opts.h"
#include "c-family/contracts-config.h"
#include "langhooks.h"

/* Instrument division by zero and INT_MIN / -1.  If not instrumenting,
   return NULL_TREE.  */

tree
ubsan_instrument_division (location_t loc, tree op0, tree op1)
{
  tree t, tt, x = NULL_TREE;
  tree type = TREE_TYPE (op0);
  enum sanitize_code flag = SANITIZE_DIVIDE;

  /* At this point both operands should have the same type,
     because they are already converted to RESULT_TYPE.
     Use TYPE_MAIN_VARIANT since typedefs can confuse us.  */
  tree top0 = TYPE_MAIN_VARIANT (type);
  tree top1 = TYPE_MAIN_VARIANT (TREE_TYPE (op1));
  gcc_checking_assert (lang_hooks.types_compatible_p (top0, top1));

  op0 = unshare_expr (op0);
  op1 = unshare_expr (op1);

  if (INTEGRAL_TYPE_P (type)
      && sanitize_flags_p (SANITIZE_DIVIDE))
    t = fold_build2 (EQ_EXPR, boolean_type_node,
		     op1, build_int_cst (type, 0));
  else if (SCALAR_FLOAT_TYPE_P (type)
	   && sanitize_flags_p (SANITIZE_FLOAT_DIVIDE))
    {
      t = fold_build2 (EQ_EXPR, boolean_type_node,
		       op1, build_real (type, dconst0));
      flag = SANITIZE_FLOAT_DIVIDE;
    }
  else
    t = NULL_TREE;

  /* We check INT_MIN / -1 only for signed types.  */
  if (INTEGRAL_TYPE_P (type)
      && sanitize_flags_p (SANITIZE_SI_OVERFLOW)
      && !TYPE_UNSIGNED (type))
    {
      tt = fold_build2 (EQ_EXPR, boolean_type_node, unshare_expr (op1),
			build_int_cst (type, -1));
      x = fold_build2 (EQ_EXPR, boolean_type_node, op0,
		       TYPE_MIN_VALUE (type));
      x = fold_build2 (TRUTH_AND_EXPR, boolean_type_node, x, tt);
      if (t == NULL_TREE || integer_zerop (t))
	{
	  t = x;
	  x = NULL_TREE;
	  flag = SANITIZE_SI_OVERFLOW;
	}
      else if ((((flag_sanitize_trap & SANITIZE_DIVIDE) == 0)
		== ((flag_sanitize_trap & SANITIZE_SI_OVERFLOW) == 0))
	       && (((flag_sanitize_recover & SANITIZE_DIVIDE) == 0)
		   == ((flag_sanitize_recover & SANITIZE_SI_OVERFLOW) == 0)))
	{
	  t = fold_build2 (TRUTH_OR_EXPR, boolean_type_node, t, x);
	  x = NULL_TREE;
	}
      else if (integer_zerop (x))
	x = NULL_TREE;
    }
  else if (t == NULL_TREE)
    return NULL_TREE;

  /* If the condition was folded to 0, no need to instrument
     this expression.  */
  if (integer_zerop (t))
    return NULL_TREE;

  /* In case we have a SAVE_EXPR in a conditional context, we need to
     make sure it gets evaluated before the condition.  */
  t = fold_build2 (COMPOUND_EXPR, TREE_TYPE (t), unshare_expr (op0), t);
  t = fold_build2 (COMPOUND_EXPR, TREE_TYPE (t), unshare_expr (op1), t);
  if ((flag_sanitize_trap & flag) && x == NULL_TREE)
    tt = build_call_expr_loc (loc, builtin_decl_explicit (BUILT_IN_TRAP), 0);
  else
    {
      tree data = ubsan_create_data ("__ubsan_overflow_data", 1, &loc,
				     ubsan_type_descriptor (type), NULL_TREE,
				     NULL_TREE);
      data = build_fold_addr_expr_loc (loc, data);
      if (flag_sanitize_trap & flag)
	tt = build_call_expr_loc (loc, builtin_decl_explicit (BUILT_IN_TRAP),
				  0);
      else
	{
	  enum built_in_function bcode
	    = (flag_sanitize_recover & flag)
	      ? BUILT_IN_UBSAN_HANDLE_DIVREM_OVERFLOW
	      : BUILT_IN_UBSAN_HANDLE_DIVREM_OVERFLOW_ABORT;
	  tt = builtin_decl_explicit (bcode);
	  op0 = unshare_expr (op0);
	  op1 = unshare_expr (op1);
	  tt = build_call_expr_loc (loc, tt, 3, data, ubsan_encode_value (op0),
				    ubsan_encode_value (op1));
	}
      if (x)
	{
	  tree xt;
	  if (flag_sanitize_trap & SANITIZE_SI_OVERFLOW)
	    xt = build_call_expr_loc (loc,
				      builtin_decl_explicit (BUILT_IN_TRAP),
				      0);
	  else
	    {
	      enum built_in_function bcode
		= (flag_sanitize_recover & SANITIZE_SI_OVERFLOW)
		   ? BUILT_IN_UBSAN_HANDLE_DIVREM_OVERFLOW
		   : BUILT_IN_UBSAN_HANDLE_DIVREM_OVERFLOW_ABORT;
	      xt = builtin_decl_explicit (bcode);
	      op0 = unshare_expr (op0);
	      op1 = unshare_expr (op1);
	      xt = build_call_expr_loc (loc, xt, 3, data,
					ubsan_encode_value (op0),
					ubsan_encode_value (op1));
	    }
	  x = fold_build3 (COND_EXPR, void_type_node, x, xt, void_node);
	}
    }
  t = fold_build3 (COND_EXPR, void_type_node, t, tt, x ? x : void_node);

  return t;
}

/* Instrument left and right shifts.  */

tree
ubsan_instrument_shift (location_t loc, enum tree_code code,
			tree op0, tree op1)
{
  tree t, tt = NULL_TREE;
  tree type0 = TREE_TYPE (op0);
  tree type1 = TREE_TYPE (op1);
  if (!INTEGRAL_TYPE_P (type0))
    return NULL_TREE;

  tree op1_utype = unsigned_type_for (type1);
  HOST_WIDE_INT op0_prec = TYPE_PRECISION (type0);
  tree uprecm1 = build_int_cst (op1_utype, op0_prec - 1);

  op0 = unshare_expr (op0);
  op1 = unshare_expr (op1);

  if (code == LROTATE_EXPR || code == RROTATE_EXPR)
    {
      /* For rotates just check for negative op1.  */
      if (TYPE_UNSIGNED (type1))
	return NULL_TREE;
      t = fold_build2 (LT_EXPR, boolean_type_node, op1,
		       build_int_cst (type1, 0));
    }
  else
    {
      t = fold_convert_loc (loc, op1_utype, op1);
      t = fold_build2 (GT_EXPR, boolean_type_node, t, uprecm1);
    }

  /* If this is not a signed operation, don't perform overflow checks.
     Also punt on bit-fields.  */
  if (TYPE_OVERFLOW_WRAPS (type0)
      || maybe_ne (GET_MODE_BITSIZE (TYPE_MODE (type0)),
		   TYPE_PRECISION (type0))
      || !sanitize_flags_p (SANITIZE_SHIFT_BASE)
      /* In C++20 and later, shifts are well defined except when
	 the second operand is not within bounds.  */
      || cxx_dialect >= cxx20)
    ;

  /* For signed x << y, in C99 and later, the following:
     (unsigned) x >> (uprecm1 - y)
     if non-zero, is undefined.  */
  else if (code == LSHIFT_EXPR && flag_isoc99 && cxx_dialect < cxx11)
    {
      tree x = fold_build2 (MINUS_EXPR, op1_utype, uprecm1,
			    fold_convert (op1_utype, unshare_expr (op1)));
      tt = fold_convert_loc (loc, unsigned_type_for (type0), op0);
      tt = fold_build2 (RSHIFT_EXPR, TREE_TYPE (tt), tt, x);
      tt = fold_build2 (NE_EXPR, boolean_type_node, tt,
			build_int_cst (TREE_TYPE (tt), 0));
    }

  /* For signed x << y, in C++11 to C++17, the following:
     x < 0 || ((unsigned) x >> (uprecm1 - y))
     if > 1, is undefined.  */
  else if (code == LSHIFT_EXPR && cxx_dialect >= cxx11)
    {
      tree x = fold_build2 (MINUS_EXPR, op1_utype, uprecm1,
			    fold_convert (op1_utype, unshare_expr (op1)));
      tt = fold_convert_loc (loc, unsigned_type_for (type0),
			     unshare_expr (op0));
      tt = fold_build2 (RSHIFT_EXPR, TREE_TYPE (tt), tt, x);
      tt = fold_build2 (GT_EXPR, boolean_type_node, tt,
			build_int_cst (TREE_TYPE (tt), 1));
      x = fold_build2 (LT_EXPR, boolean_type_node, unshare_expr (op0),
		       build_int_cst (type0, 0));
      tt = fold_build2 (TRUTH_OR_EXPR, boolean_type_node, x, tt);
    }

  /* If the condition was folded to 0, no need to instrument
     this expression.  */
  if (integer_zerop (t) && (tt == NULL_TREE || integer_zerop (tt)))
    return NULL_TREE;

  /* In case we have a SAVE_EXPR in a conditional context, we need to
     make sure it gets evaluated before the condition.  */
  t = fold_build2 (COMPOUND_EXPR, TREE_TYPE (t), unshare_expr (op0), t);
  t = fold_build2 (COMPOUND_EXPR, TREE_TYPE (t), unshare_expr (op1), t);

  enum sanitize_code recover_kind = SANITIZE_SHIFT_EXPONENT;
  tree else_t = void_node;
  if (tt)
    {
      if (!sanitize_flags_p (SANITIZE_SHIFT_EXPONENT))
	{
	  t = fold_build1 (TRUTH_NOT_EXPR, boolean_type_node, t);
	  t = fold_build2 (TRUTH_AND_EXPR, boolean_type_node, t, tt);
	  recover_kind = SANITIZE_SHIFT_BASE;
	}
      else
	{
	  if (((!(flag_sanitize_trap & SANITIZE_SHIFT_EXPONENT))
	       == (!(flag_sanitize_trap & SANITIZE_SHIFT_BASE)))
	      && ((!(flag_sanitize_recover & SANITIZE_SHIFT_EXPONENT))
		  == (!(flag_sanitize_recover & SANITIZE_SHIFT_BASE))))
	    t = fold_build2 (TRUTH_OR_EXPR, boolean_type_node, t, tt);
	  else
	    else_t = tt;
	}
    }

  if ((flag_sanitize_trap & recover_kind) && else_t == void_node)
    tt = build_call_expr_loc (loc, builtin_decl_explicit (BUILT_IN_TRAP), 0);
  else
    {
      if (TREE_CODE (type1) == BITINT_TYPE
	  && TYPE_PRECISION (type1) > MAX_FIXED_MODE_SIZE)
	{
	  /* Workaround for missing _BitInt support in libsanitizer.
	     Instead of crashing in the library, pretend values above
	     maximum value of normal integral type or below minimum value
	     of that type are those extremes.  */
	  tree type2 = build_nonstandard_integer_type (MAX_FIXED_MODE_SIZE,
						       TYPE_UNSIGNED (type1));
	  tree op2 = op1;
	  if (!TYPE_UNSIGNED (type1))
	    {
	      op2 = fold_build2 (LT_EXPR, boolean_type_node, unshare_expr (op1),
				 fold_convert (type1, TYPE_MIN_VALUE (type2)));
	      op2 = fold_build3 (COND_EXPR, type2, op2, TYPE_MIN_VALUE (type2),
				 fold_convert (type2, unshare_expr (op1)));
	    }
	  else
	    op2 = fold_convert (type2, op1);
	  tree op3
	    = fold_build2 (GT_EXPR, boolean_type_node, unshare_expr (op1),
			   fold_convert (type1, TYPE_MAX_VALUE (type2)));
	  op1 = fold_build3 (COND_EXPR, type2, op3, TYPE_MAX_VALUE (type2),
			     op2);
	  type1 = type2;
	}
      tree utd0 = ubsan_type_descriptor (type0, UBSAN_PRINT_FORCE_INT);
      tree data = ubsan_create_data ("__ubsan_shift_data", 1, &loc, utd0,
				     ubsan_type_descriptor (type1), NULL_TREE,
				     NULL_TREE);
      data = build_fold_addr_expr_loc (loc, data);

      if (flag_sanitize_trap & recover_kind)
	tt = build_call_expr_loc (loc, builtin_decl_explicit (BUILT_IN_TRAP), 0);
      else
	{
	  enum built_in_function bcode
	    = (flag_sanitize_recover & recover_kind)
	      ? BUILT_IN_UBSAN_HANDLE_SHIFT_OUT_OF_BOUNDS
	      : BUILT_IN_UBSAN_HANDLE_SHIFT_OUT_OF_BOUNDS_ABORT;
	  tt = builtin_decl_explicit (bcode);
	  op0 = unshare_expr (op0);
	  op1 = unshare_expr (op1);
	  tt = build_call_expr_loc (loc, tt, 3, data, ubsan_encode_value (op0),
				    ubsan_encode_value (op1));
	}
      if (else_t != void_node)
	{
	  tree else_tt;
	  if (flag_sanitize_trap & SANITIZE_SHIFT_BASE)
	    else_tt
	      = build_call_expr_loc (loc,
				     builtin_decl_explicit (BUILT_IN_TRAP), 0);
	  else
	    {
	      enum built_in_function bcode
		= (flag_sanitize_recover & SANITIZE_SHIFT_BASE)
		  ? BUILT_IN_UBSAN_HANDLE_SHIFT_OUT_OF_BOUNDS
		  : BUILT_IN_UBSAN_HANDLE_SHIFT_OUT_OF_BOUNDS_ABORT;
	      else_tt = builtin_decl_explicit (bcode);
	      op0 = unshare_expr (op0);
	      op1 = unshare_expr (op1);
	      else_tt = build_call_expr_loc (loc, else_tt, 3, data,
					     ubsan_encode_value (op0),
					     ubsan_encode_value (op1));
	    }
	  else_t = fold_build3 (COND_EXPR, void_type_node, else_t,
				else_tt, void_node);
	}
    }
  t = fold_build3 (COND_EXPR, void_type_node, t, tt, else_t);

  return t;
}

/* Instrument variable length array bound.  */

tree
ubsan_instrument_vla (location_t loc, tree size)
{
  tree type = TREE_TYPE (size);
  tree t, tt;

  t = fold_build2 (LE_EXPR, boolean_type_node, size, build_int_cst (type, 0));
  if (flag_sanitize_trap & SANITIZE_VLA)
    tt = build_call_expr_loc (loc, builtin_decl_explicit (BUILT_IN_TRAP), 0);
  else
    {
      tree data = ubsan_create_data ("__ubsan_vla_data", 1, &loc,
				     ubsan_type_descriptor (type), NULL_TREE,
				     NULL_TREE);
      data = build_fold_addr_expr_loc (loc, data);
      enum built_in_function bcode
	= (flag_sanitize_recover & SANITIZE_VLA)
	  ? BUILT_IN_UBSAN_HANDLE_VLA_BOUND_NOT_POSITIVE
	  : BUILT_IN_UBSAN_HANDLE_VLA_BOUND_NOT_POSITIVE_ABORT;
      tt = builtin_decl_explicit (bcode);
      tt = build_call_expr_loc (loc, tt, 2, data, ubsan_encode_value (size));
    }
  t = fold_build3 (COND_EXPR, void_type_node, t, tt, void_node);

  return t;
}

/* Instrument missing return in C++ functions returning non-void.  */

tree
ubsan_instrument_return (location_t loc)
{
  if (flag_sanitize_trap & SANITIZE_RETURN)
    /* pass_warn_function_return checks for BUILTINS_LOCATION.  */
    return build_call_expr_loc (BUILTINS_LOCATION,
				builtin_decl_explicit (BUILT_IN_TRAP), 0);

  tree data = ubsan_create_data ("__ubsan_missing_return_data", 1, &loc,
				 NULL_TREE, NULL_TREE);
  tree t = builtin_decl_explicit (BUILT_IN_UBSAN_HANDLE_MISSING_RETURN);
  return build_call_expr_loc (loc, t, 1, build_fold_addr_expr_loc (loc, data));
}

/* Get the tree that represented the number of counted_by, i.e, the maximum
   number of the elements of the object that the call to .ACCESS_WITH_SIZE
   points to, this number will be the bound of the corresponding array.  */
static tree
get_bound_from_access_with_size (tree call)
{
  if (!is_access_with_size_p (call))
    return NULL_TREE;

  tree ref_to_size = CALL_EXPR_ARG (call, 1);
  tree type = TREE_TYPE (TREE_TYPE (CALL_EXPR_ARG (call, 2)));
  tree size = fold_build2 (MEM_REF, type, unshare_expr (ref_to_size),
			   build_int_cst (ptr_type_node, 0));
  /* If size is negative value, treat it as zero.  */
  if (!TYPE_UNSIGNED (type))
  {
    tree cond = fold_build2 (LT_EXPR, boolean_type_node,
			     unshare_expr (size), build_zero_cst (type));
    size = fold_build3 (COND_EXPR, type, cond,
			build_zero_cst (type), size);
  }

  size = fold_convert (sizetype, size);

  return size;
}


/* Instrument array bounds for ARRAY_REFs.  We create special builtin,
   that gets expanded in the sanopt pass, and make an array dimension
   of it.  ARRAY is the array, *INDEX is an index to the array.
   Return NULL_TREE if no instrumentation is emitted.
   IGNORE_OFF_BY_ONE is true if the ARRAY_REF is inside an ADDR_EXPR.  */

tree
ubsan_instrument_bounds (location_t loc, tree array, tree *index,
			 bool ignore_off_by_one)
{
  tree type = TREE_TYPE (array);
  tree domain = TYPE_DOMAIN (type);

  if (domain == NULL_TREE)
    return NULL_TREE;

  tree bound = TYPE_MAX_VALUE (domain);
  if (!bound)
    {
      /* Handle C [0] arrays, which have TYPE_MAX_VALUE NULL, like
	 C++ [0] arrays which have TYPE_MIN_VALUE 0 TYPE_MAX_VALUE -1.  */
      if (!c_dialect_cxx ()
	  && COMPLETE_TYPE_P (type)
	  && integer_zerop (TYPE_SIZE (type)))
	bound = build_int_cst (TREE_TYPE (TYPE_MIN_VALUE (domain)), -1);
      else if (INDIRECT_REF_P (array)
	       && is_access_with_size_p ((TREE_OPERAND (array, 0))))
	{
	  bound = get_bound_from_access_with_size ((TREE_OPERAND (array, 0)));
	  bound = fold_build2 (MINUS_EXPR, TREE_TYPE (bound),
			       bound,
			       build_int_cst (TREE_TYPE (bound), 1));
	}
      else
	return NULL_TREE;
    }

  bound = fold_build2 (PLUS_EXPR, TREE_TYPE (bound), bound,
		       build_int_cst (TREE_TYPE (bound),
				      1 + ignore_off_by_one));

  /* Detect flexible array members and suchlike, unless
     -fsanitize=bounds-strict.  */
  tree base = get_base_address (array);
  if (!sanitize_flags_p (SANITIZE_BOUNDS_STRICT)
      && TREE_CODE (array) == COMPONENT_REF
      && base && (INDIRECT_REF_P (base) || TREE_CODE (base) == MEM_REF))
    {
      tree next = NULL_TREE;
      tree cref = array;

      /* Walk all structs/unions.  */
      while (TREE_CODE (cref) == COMPONENT_REF)
	{
	  if (TREE_CODE (TREE_TYPE (TREE_OPERAND (cref, 0))) == RECORD_TYPE)
	    for (next = DECL_CHAIN (TREE_OPERAND (cref, 1));
		 next && TREE_CODE (next) != FIELD_DECL;
		 next = DECL_CHAIN (next))
	      ;
	  if (next)
	    /* Not a last element.  Instrument it.  */
	    break;
	  if (TREE_CODE (TREE_TYPE (TREE_OPERAND (cref, 1))) == ARRAY_TYPE
	      && !c_dialect_cxx ())
	    {
	      unsigned l
		= c_strict_flex_array_level_of (TREE_OPERAND (cref, 1));
	      tree type2 = TREE_TYPE (TREE_OPERAND (cref, 1));
	      if (TYPE_DOMAIN (type2) != NULL_TREE)
		{
		  tree max = TYPE_MAX_VALUE (TYPE_DOMAIN (type2));
		  if (max == NULL_TREE)
		    {
		      /* C [0] */
		      if (COMPLETE_TYPE_P (type2)
			  && integer_zerop (TYPE_SIZE (type2))
			  && l == 3)
			next = TREE_OPERAND (cref, 1);
		    }
		  else if (TREE_CODE (max) == INTEGER_CST)
		    {
		      if (c_dialect_cxx ()
			  && integer_all_onesp (max))
			{
			  /* C++ [0] */
			  if (l == 3)
			    next = TREE_OPERAND (cref, 1);
			}
		      else if (integer_zerop (max))
			{
			  /* C/C++ [1] */
			  if (l >= 2)
			    next = TREE_OPERAND (cref, 1);
			}
		      else if (l >= 1)
			next = TREE_OPERAND (cref, 1);
		    }
		}
	      if (next)
		break;
	    }
	  /* Ok, this is the last field of the structure/union.  But the
	     aggregate containing the field must be the last field too,
	     recursively.  */
	  cref = TREE_OPERAND (cref, 0);
	}
      if (!next)
	/* Don't instrument this flexible array member-like array in non-strict
	   -fsanitize=bounds mode.  */
        return NULL_TREE;
    }

  /* Don't emit instrumentation in the most common cases.  */
  tree idx = NULL_TREE;
  if (TREE_CODE (*index) == INTEGER_CST)
    idx = *index;
  else if (TREE_CODE (*index) == BIT_AND_EXPR
	   && TREE_CODE (TREE_OPERAND (*index, 1)) == INTEGER_CST)
    idx = TREE_OPERAND (*index, 1);
  if (idx
      && TREE_CODE (bound) == INTEGER_CST
      && tree_int_cst_sgn (idx) >= 0
      && tree_int_cst_lt (idx, bound))
    return NULL_TREE;

  *index = save_expr (*index);
  /* If TYPE is a VLA, use 1 instead of 0 as the first argument and
     use just the addend to TYPE_MAX_VALUE (domain) as the third argument
     temporarily, so that gimplification can use TYPE_MAX_VALUE (domain)
     after gimplify_type_sizes.  See PR120052.  */
  bool is_vla = (TYPE_MAX_VALUE (domain)
		 && TREE_CODE (TYPE_MAX_VALUE (domain)) != INTEGER_CST);
  if (is_vla)
    bound = build_int_cst (TREE_TYPE (bound), 1 + ignore_off_by_one);
  /* Create a "(T *) 0" (or 1) tree node to describe the array type.  */
  tree zero_with_type = build_int_cst (build_pointer_type (type), is_vla);
  return build_call_expr_internal_loc (loc, IFN_UBSAN_BOUNDS,
				       void_type_node, 3, zero_with_type,
				       *index, bound);
}


/* Instrument array bounds for the pointer array address which is
   a call to .ACCESS_WITH_SIZE.  We create special
   builtin, that gets expanded in the sanopt pass, and make an array
   dimension of it.  POINTER_ADDR is the pointer array's base address.
   *INDEX is an index to the array.
   IGNORE_OFF_BY_ONE is true if the POINTER_ADDR is not inside an
   INDIRECT_REF.
   Return NULL_TREE if no instrumentation is emitted.  */

tree
ubsan_instrument_bounds_pointer_address (location_t loc, tree pointer_addr,
					 tree *index,
					 bool ignore_off_by_one)
{
  tree call = pointer_addr;
  if (!is_access_with_size_p (call))
    return NULL_TREE;
  tree bound = get_bound_from_access_with_size (call);

  if (ignore_off_by_one)
    bound = fold_build2 (PLUS_EXPR, TREE_TYPE (bound), bound,
			 build_int_cst (TREE_TYPE (bound),
			 1));

  /* Don't emit instrumentation in the most common cases.  */
  tree idx = NULL_TREE;
  if (TREE_CODE (*index) == INTEGER_CST)
    idx = *index;
  else if (TREE_CODE (*index) == BIT_AND_EXPR
	   && TREE_CODE (TREE_OPERAND (*index, 1)) == INTEGER_CST)
    idx = TREE_OPERAND (*index, 1);
  if (idx
      && TREE_CODE (bound) == INTEGER_CST
      && tree_int_cst_sgn (idx) >= 0
      && tree_int_cst_lt (idx, bound))
    return NULL_TREE;

  *index = save_expr (*index);

  /* Create an array_type for the corresponding pointer array.  */
  tree itype = build_index_type (NULL_TREE);
  /* The array's element type can be get from the return type of the call to
     .ACCESS_WITH_SIZE.  */
  tree element_type = TREE_TYPE (TREE_TYPE (call));
  tree array_type = build_array_type (element_type, itype);
  /* Create a "(T *) 0" tree node to describe the array type.  */
  tree zero_with_type = build_int_cst (build_pointer_type (array_type), 0);
  return build_call_expr_internal_loc (loc, IFN_UBSAN_BOUNDS,
				       void_type_node, 3, zero_with_type,
				       *index, bound);
}

/* This structure is to combine a factor with its parent and its position
 * in its parent tree.  */
struct factor_t
{
  tree factor;
  tree parent; /* the parent tree of this factor.  */
  int pos;     /* the position of this factor in its parent tree.  */
};

/* for a multiply expression like:
    ((long unsigned int) m * (long unsigned int) SAVE_EXPR <n>) * 4

   locate all the factors, the parents of the factor and the position of
   the factor in its parent, and put them to VEC_FACTORS.  */

static void
get_factors_from_mul_expr (tree mult_expr, tree parent,
			   int pos, auto_vec<factor_t> *vec_factors)
{
  struct factor_t mult_factor = {0, 0, -1};
  mult_factor.factor = mult_expr;
  mult_factor.parent = parent;
  mult_factor.pos = pos;

  while (CONVERT_EXPR_CODE_P (TREE_CODE (mult_expr)))
    {
      mult_factor.parent = mult_expr;
      mult_factor.pos = 0;
      mult_expr = TREE_OPERAND (mult_expr, 0);
      mult_factor.factor = mult_expr;
    }
  if (TREE_CODE (mult_expr) != MULT_EXPR)
    vec_factors->safe_push (mult_factor);
  else
    {
      get_factors_from_mul_expr (TREE_OPERAND (mult_expr, 0), mult_expr,
				 0, vec_factors);
      get_factors_from_mul_expr (TREE_OPERAND (mult_expr, 1), mult_expr,
				 1, vec_factors);
    }
}

/* Given an OFFSET expression, and the ELEMENT_SIZE,
   get the index expression from OFFSET and return it.
   For example:
   OFFSET:
    ((long unsigned int) m * (long unsigned int) SAVE_EXPR <n>) * 4
   ELEMENT_SIZE:
    (sizetype) SAVE_EXPR <n> * 4
   get the index as (long unsigned int) m, and return it.
   One special case is when the OFFSET is an integer constant, and the
   element_size is also an integer constant, we should get the index
   as OFFSET/element_size.
   The INDEX_P holds the pointer to the parent tree of the index,
   INDEX_N holds the position of the index in its parent.  */

static tree
get_index_from_offset (tree offset, tree *index_p,
		       int *index_n, tree *factor,
		       tree element_size)
{
  if (TREE_CODE (offset) != MULT_EXPR
      && TREE_CODE (offset) != INTEGER_CST)
    return NULL_TREE;

  if (TREE_CODE (offset) == INTEGER_CST
      && TREE_CODE (element_size) != INTEGER_CST)
    return NULL_TREE;

  if (TREE_CODE (offset) == INTEGER_CST
      && TREE_CODE (element_size) == INTEGER_CST)
    {
      *factor = element_size;
      return fold_build2 (EXACT_DIV_EXPR, TREE_TYPE (offset),
			  offset, element_size);
    }

  auto_vec<factor_t> e_factors, o_factors;
  get_factors_from_mul_expr (element_size, NULL, -1, &e_factors);
  get_factors_from_mul_expr (offset, *index_p, *index_n, &o_factors);

  if (e_factors.is_empty () || o_factors.is_empty ())
    return NULL_TREE;

  bool all_found = true;
  for (unsigned i = 0; i < e_factors.length (); i++)
    {
      factor_t e_size_factor = e_factors[i];
      bool found = false;
      for (unsigned j = 0; j < o_factors.length ();)
	{
	  factor_t o_exp_factor = o_factors[j];
	  if (operand_equal_p (e_size_factor.factor, o_exp_factor.factor))
	    {
	      o_factors.unordered_remove (j);
	      found = true;
	      break;
	    }
	  else
	    j++;
	}
      if (!found)
	all_found = false;
    }

  if (!all_found)
    return NULL_TREE;

  if (o_factors.length () != 1)
    return NULL_TREE;

  *index_p = o_factors[0].parent;
  *index_n = o_factors[0].pos;
  return o_factors[0].factor;
}

/* For an pointer + offset computation expression, such as,
   .ACCESS_WITH_SIZE (p->c, &p->b, 1, 0, -1, 0B)
    + (sizetype) ((long unsigned int) index * 4
   Return the index of this pointer array reference,
   set the parent tree of INDEX to *INDEX_P.
   set the operand position of the INDEX in the parent tree to *INDEX_N.
   If failed, return NULL_TREE.  */

static tree
get_index_from_pointer_addr_expr (tree pointer, tree *index_p,
				  int *index_n, tree *factor)
{
  *index_p = NULL_TREE;
  *index_n = -1;
  tree call = TREE_OPERAND (pointer, 0);
  if (!is_access_with_size_p (call))
    return NULL_TREE;

  /* Get the pointee type of the call to .ACCESS_WITH_SIZE.
     This should be the element type of the pointer array.  */
  tree pointee_type = TREE_TYPE (TREE_TYPE (call));
  if (!COMPLETE_OR_VOID_TYPE_P (pointee_type))
    return NULL_TREE;
  tree pointee_size
    = (VOID_TYPE_P (pointee_type)
       ? size_one_node : TYPE_SIZE_UNIT (pointee_type));

  tree index_exp = TREE_OPERAND (pointer, 1);
  *index_p = pointer;
  *index_n = 1;

  if (!(TREE_CODE (index_exp) != MULT_EXPR
	&& tree_int_cst_equal (pointee_size, integer_one_node)))
    {
      while (CONVERT_EXPR_P (index_exp))
	{
	  *index_p = index_exp;
	  *index_n = 0;
	  index_exp = TREE_OPERAND (index_exp, 0);
	}
      index_exp = get_index_from_offset (index_exp, index_p,
					 index_n, factor, pointee_size);

      if (!index_exp)
	return NULL_TREE;
    }

  while (CONVERT_EXPR_P (index_exp))
    {
      *index_p = index_exp;
      *index_n = 0;
      index_exp = TREE_OPERAND (index_exp, 0);
    }

  return index_exp;
}

/* Return TRUE when the EXPR is a pointer array address that could be
   instrumented.
   We only instrument an address computation similar as the following:
      .ACCESS_WITH_SIZE (p->c, &p->b, 1, 0, -1, 0B)
	+ (sizetype) ((long unsigned int) index * 4)
   if the EXPR is instrumentable, return TRUE and
   set the index to *INDEX.
   set the .ACCESS_WITH_SIZE to *BASE.
   set the parent tree of INDEX to *INDEX_P.
   set the operand position of the INDEX in the parent tree to INDEX_N.  */

static bool
is_instrumentable_pointer_array_address (tree expr, tree *base,
					 tree *index, tree *index_p,
					 int *index_n, tree *factor)
{
  /* For a pointer array address as:
	.ACCESS_WITH_SIZE (p->c, &p->b, 1, 0, -1, 0B)
	  + (sizetype) ((long unsigned int) index * 4)
     op0 is the call to .ACCESS_WITH_SIZE;
     op1 is the index.  */
  if (TREE_CODE (expr) != POINTER_PLUS_EXPR)
    return false;

  tree op0 = TREE_OPERAND (expr, 0);
  if (!is_access_with_size_p (op0))
    return false;
  tree op1 = get_index_from_pointer_addr_expr (expr, index_p, index_n, factor);
  if (op1 != NULL_TREE)
    {
      *base = op0;
      *index = op1;
      return true;
    }
  return false;
}

/* Return true iff T is an array or an indirect reference that was
   instrumented by SANITIZE_BOUNDS.  */

bool
ubsan_array_ref_instrumented_p (tree t)
{
  if (TREE_CODE (t) != ARRAY_REF
      && TREE_CODE (t) != MEM_REF)
    return false;

  bool is_array = (TREE_CODE (t) == ARRAY_REF);
  tree op0 = NULL_TREE;
  tree op1 = NULL_TREE;
  tree index_p = NULL_TREE;
  tree factor = NULL_TREE;
  int index_n = 0;

  if (is_array)
    op1 = TREE_OPERAND (t, 1);
  else if (is_instrumentable_pointer_array_address (t, &op0, &op1,
						    &index_p, &index_n,
						    &factor))
    {
    }

  return op1 != NULL_TREE
	 && TREE_CODE (op1) == COMPOUND_EXPR
	 && TREE_CODE (TREE_OPERAND (op1, 0)) == CALL_EXPR
	 && CALL_EXPR_FN (TREE_OPERAND (op1, 0)) == NULL_TREE
	 && CALL_EXPR_IFN (TREE_OPERAND (op1, 0)) == IFN_UBSAN_BOUNDS;
}

/* P3100: whether ARRAY, the array operand of an ARRAY_REF, is a trailing
   array member -- of an object accessed through a pointer, and of each
   enclosing aggregate in turn -- that may be used as a flexible array
   member, and so has no bound to check.  -fstrict-flex-arrays decides which
   sizes qualify, in C++ as in C.  The walk is ubsan_instrument_bounds's,
   which applies the levels in C only.  */

bool
p3100_flexible_array_member_like_p (tree array)
{
  tree base = get_base_address (array);
  if (TREE_CODE (array) != COMPONENT_REF
      || !base || !(INDIRECT_REF_P (base) || TREE_CODE (base) == MEM_REF))
    return false;

  tree next = NULL_TREE;
  for (tree cref = array; TREE_CODE (cref) == COMPONENT_REF;
       cref = TREE_OPERAND (cref, 0))
    {
      tree field = TREE_OPERAND (cref, 1);
      if (TREE_CODE (TREE_TYPE (TREE_OPERAND (cref, 0))) == RECORD_TYPE)
	for (next = DECL_CHAIN (field);
	     next && TREE_CODE (next) != FIELD_DECL;
	     next = DECL_CHAIN (next))
	  ;
      if (next)
	/* Not a last element.  */
	break;
      tree type = TREE_TYPE (field);
      if (TREE_CODE (type) == ARRAY_TYPE && TYPE_DOMAIN (type) != NULL_TREE)
	{
	  unsigned l = c_strict_flex_array_level_of (field);
	  tree max = TYPE_MAX_VALUE (TYPE_DOMAIN (type));
	  if (max == NULL_TREE)
	    {
	      /* C [0] */
	      if (COMPLETE_TYPE_P (type) && integer_zerop (TYPE_SIZE (type))
		  && l == 3)
		next = field;
	    }
	  else if (TREE_CODE (max) == INTEGER_CST)
	    {
	      if (c_dialect_cxx () && integer_all_onesp (max))
		{
		  /* C++ [0] */
		  if (l == 3)
		    next = field;
		}
	      else if (integer_zerop (max))
		{
		  /* C/C++ [1] */
		  if (l >= 2)
		    next = field;
		}
	      else if (l >= 1)
		next = field;
	    }
	  if (next)
	    break;
	}
      /* The last field of its aggregate, which must in turn be the last
	 field of the one containing it.  */
    }
  return !next;
}

/* P3100: guard the indirection *EXPR_P, an INDIRECT_REF, when its operand is
   pointer arithmetic on an array of statically-known size decayed to a pointer
   to its first element (*(a + i)): the byte offset must stay below the size of
   the array, since an indirection through the pointer past its end is
   undefined ({expr.unary.dereference.invalid.known}).  The arithmetic itself
   was checked against the one-past bound where it was built.  The offset is
   replaced with a guarded one, the defined valid offset 0 when out of range;
   nothing happens unless the site resolves to a semantic other than
   assume.  */

void
ubsan_maybe_instrument_array_indirection (tree *expr_p)
{
  if (!flag_contracts_p3100 || current_function_decl == NULL_TREE)
    return;
  tree ptr = TREE_OPERAND (*expr_p, 0);
  if (TREE_CODE (ptr) != POINTER_PLUS_EXPR)
    return;
  tree base = tree_strip_nop_conversions (TREE_OPERAND (ptr, 0));
  if (TREE_CODE (base) != ADDR_EXPR)
    return;
  tree array = TREE_OPERAND (base, 0);
  tree atype = TREE_TYPE (array);
  if (TREE_CODE (atype) != ARRAY_TYPE)
    return;
  tree domain = TYPE_DOMAIN (atype);
  tree maxv = domain ? TYPE_MAX_VALUE (domain) : NULL_TREE;
  tree size = TYPE_SIZE_UNIT (atype);
  if (maxv == NULL_TREE || TREE_CODE (maxv) != INTEGER_CST
      || size == NULL_TREE || TREE_CODE (size) != INTEGER_CST
      || integer_zerop (size)
      || p3100_flexible_array_member_like_p (array))
    return;

  tree off = TREE_OPERAND (ptr, 1);
  if (TREE_CODE (off) == INTEGER_CST
      && tree_int_cst_lt (fold_convert (sizetype, off),
			  fold_convert (sizetype, size)))
    return;
  location_t loc = EXPR_LOCATION (*expr_p);
  if (loc == UNKNOWN_LOCATION)
    loc = EXPR_LOCATION (ptr);
  tree g = lang_hooks.build_implicit_bounds_check
	     (current_function_decl, loc, off, fold_convert (sizetype, size),
	      "ub:expr.unary.dereference.invalid.known");
  if (g != NULL_TREE)
    TREE_OPERAND (ptr, 1) = g;
}

/* Instrument an ARRAY_REF or an address computation whose base address is
   a call to .ACCESS_WITH_SIZE, if it hasn't already been instrumented.
   IGNORE_OFF_BY_ONE is true if the ARRAY_REF is inside a ADDR_EXPR, or the
   address computation is not inside a INDIRECT_REF.  */

void
ubsan_maybe_instrument_array_ref (tree *expr_p, bool ignore_off_by_one)
{
  tree op0 = NULL_TREE;
  tree op1 = NULL_TREE;
  tree index_p = NULL_TREE;  /* the parent tree of INDEX.  */
  tree factor = NULL_TREE;
  int index_n = 0;  /* the operand position of INDEX in the parent tree.  */

  bool do_sanitize
    = sanitize_flags_p (SANITIZE_BOUNDS | SANITIZE_BOUNDS_STRICT);
  if (!ubsan_array_ref_instrumented_p (*expr_p)
      && (do_sanitize || flag_contracts_p3100)
      && current_function_decl != NULL_TREE)
    {
      /* P3100 implicit array-bounds contract assertion.  It takes precedence
	 over the sanitizer, which checks the subscript only when no contract
	 applies.  */
      bool contract_applied = false;
      if (TREE_CODE (*expr_p) == ARRAY_REF && flag_contracts_p3100)
	{
	  /* Only the statically-known-bound case is handled: the array
	     operand has an ARRAY_TYPE with a constant upper domain bound.  The
	     C++ langhook resolves the per-site semantic and builds a guarded
	     index (out-of-range -> the defined valid index 0) that REPLACES the
	     original index outright.  */
	  op0 = TREE_OPERAND (*expr_p, 0);
	  op1 = TREE_OPERAND (*expr_p, 1);
	  tree atype = TREE_TYPE (op0);
	  tree domain = (TREE_CODE (atype) == ARRAY_TYPE
			 ? TYPE_DOMAIN (atype) : NULL_TREE);
	  tree maxv = domain ? TYPE_MAX_VALUE (domain) : NULL_TREE;
	  /* A trailing array that may be used as a flexible array member has
	     no bound to check, rather than one to redirect every access
	     to.  */
	  if (maxv != NULL_TREE && TREE_CODE (maxv) == INTEGER_CST
	      && !p3100_flexible_array_member_like_p (op0))
	    {
	      /* First out-of-range index = max + 1 (+ 1 more when a one-past
		 address is being formed rather than an element accessed).  */
	      tree bound
		= fold_build2 (PLUS_EXPR, TREE_TYPE (maxv), maxv,
			       build_int_cst (TREE_TYPE (maxv),
					      1 + ignore_off_by_one));
	      /* Skip a constant index that is provably in range.  */
	      if (!(TREE_CODE (op1) == INTEGER_CST
		    && tree_int_cst_sgn (op1) >= 0
		    && tree_int_cst_lt (op1, bound)))
		{
		  tree g = lang_hooks.build_implicit_bounds_check
			     (current_function_decl, EXPR_LOCATION (*expr_p),
			      op1, bound,
			      "ub:expr.add.out.of.bounds.known.subscript");
		  if (g != NULL_TREE)
		    {
		      TREE_OPERAND (*expr_p, 1) = g;
		      contract_applied = true;
		    }
		}
	      /* The one-past index of an address (&a[N]) is not an
		 out-of-bounds subscript, but a[N] is an indirection through a
		 pointer past the end of the array, which C++ does not allow
		 even when only its address is taken.  Guard the (already
		 guarded) index against the element count, so that an index
		 the subscript check redirected is not reported twice.  The
		 sanitizer, which follows C here, still checks the subscript
		 when no contract applies to it.  */
	      if (ignore_off_by_one)
		{
		  tree idx = TREE_OPERAND (*expr_p, 1);
		  tree nelts
		    = fold_build2 (PLUS_EXPR, TREE_TYPE (maxv), maxv,
				   build_int_cst (TREE_TYPE (maxv), 1));
		  if (!(TREE_CODE (idx) == INTEGER_CST
			&& tree_int_cst_sgn (idx) >= 0
			&& tree_int_cst_lt (idx, nelts)))
		    {
		      tree g = lang_hooks.build_implicit_bounds_check
				 (current_function_decl,
				  EXPR_LOCATION (*expr_p), idx, nelts,
				  "ub:expr.unary.dereference.invalid.known");
		      if (g != NULL_TREE)
			TREE_OPERAND (*expr_p, 1) = g;
		    }
		}
	    }
	}
      if (contract_applied)
	;
      else if (TREE_CODE (*expr_p) == ARRAY_REF && do_sanitize)
	{
	  op0 = TREE_OPERAND (*expr_p, 0);
	  op1 = TREE_OPERAND (*expr_p, 1);
	  tree e = ubsan_instrument_bounds (EXPR_LOCATION (*expr_p), op0,
					    &op1, ignore_off_by_one);
	  /* Replace the original INDEX with the instrumented INDEX.  */
	  if (e != NULL_TREE)
	    TREE_OPERAND (*expr_p, 1)
	      = build2 (COMPOUND_EXPR, TREE_TYPE (op1), e, op1);
	}
      else if (do_sanitize
	       && is_instrumentable_pointer_array_address (*expr_p, &op0, &op1,
							   &index_p, &index_n,
							   &factor))
	{
	  tree e
	    = ubsan_instrument_bounds_pointer_address (EXPR_LOCATION (*expr_p),
						       op0, &op1,
						       ignore_off_by_one);
	   /* Replace the original INDEX with the instrumented INDEX.  */
	  if (e != NULL_TREE)
	    {
	      /* If we had to remove a constant factor, add it back.  */
	      if (factor != NULL_TREE)
		op1 = fold_build2 (MULT_EXPR, TREE_TYPE (op1), op1, factor);

	      TREE_OPERAND (index_p, index_n)
		= build2 (COMPOUND_EXPR, TREE_TYPE (op1), e, op1);
	    }
	}
    }
}

/* P3100: whether OP, the pointer a reference is bound to or a member
   function is called on, designates a member subobject whose object pointer
   is checked for null already: the address of a member, or of an element of
   a member array, of an object reached through a pointer, possibly adjusted
   by an offset ((p->s.*pmf) ()), whose address computation the ubsan pass
   checks on that pointer (instrument_null).  The address is null only when
   the object pointer is and the member's offset is zero, so a check of it
   would report the same null dereference again.  (An address formed from
   the object pointer of OP->*PM is recognized by the ubsan pass,
   ubsan_ptrmem_derived_p.)  Also an address the front end formed from an
   object pointer it checked, the check first -- a virtual base class
   subobject of `*p' (cp_ubsan_maybe_check_vbase_conversion).  */

static bool
ubsan_p3100_checked_subobject_p (tree op)
{
  STRIP_NOPS (op);
  if (TREE_CODE (op) == COMPOUND_EXPR)
    {
      tree call = TREE_OPERAND (op, 0);
      if (TREE_CODE (call) == CALL_EXPR
	  && CALL_EXPR_FN (call) == NULL_TREE
	  && CALL_EXPR_IFN (call) == IFN_UBSAN_NULL
	  && (tree_to_uhwi (CALL_EXPR_ARG (call, UBSAN_NULL_CKIND))
	      == UBSAN_MEMBER_ACCESS))
	return true;
    }
  while (true)
    {
      STRIP_NOPS (op);
      if (TREE_CODE (op) == SAVE_EXPR || TREE_CODE (op) == POINTER_PLUS_EXPR)
	op = TREE_OPERAND (op, 0);
      else
	break;
    }
  if (TREE_CODE (op) != ADDR_EXPR
      || !handled_component_p (TREE_OPERAND (op, 0)))
    return false;
  tree base = TREE_OPERAND (op, 0);
  while (handled_component_p (base))
    base = TREE_OPERAND (base, 0);
  if (!INDIRECT_REF_P (base) && TREE_CODE (base) != MEM_REF)
    return false;
  /* The ubsan pass checks a pointer that is not a constant: a null
     constant is checked here, or not at all.  */
  tree ptr = TREE_OPERAND (base, 0);
  STRIP_NOPS (ptr);
  return TREE_CODE (ptr) != ADDR_EXPR && !CONSTANT_CLASS_P (ptr);
}

/* P3100: whether OP, the pointer a reference is bound to or a member
   function is called on, is formed from a glvalue that is a reference
   already -- a reference variable or parameter, a reference member, a call
   returning a reference -- by conversions, base class adjustments or member
   accesses: `const T &r2 = r', `B &b = d' for `D &d', `r.f ()',
   `sid (*sp).f ()'.  That indirects through no pointer: the object was
   reached where the reference was bound, and checked there.  */

static bool
ubsan_p3100_reference_glvalue_p (tree op)
{
  while (true)
    {
      STRIP_NOPS (op);
      if (TREE_CODE (op) == SAVE_EXPR || TREE_CODE (op) == POINTER_PLUS_EXPR)
	op = TREE_OPERAND (op, 0);
      else if (TREE_CODE (op) == ADDR_EXPR
	       && handled_component_p (TREE_OPERAND (op, 0)))
	{
	  tree base = TREE_OPERAND (op, 0);
	  while (handled_component_p (base))
	    base = TREE_OPERAND (base, 0);
	  if (!INDIRECT_REF_P (base))
	    return false;
	  op = TREE_OPERAND (base, 0);
	}
      else
	break;
    }
  return (TREE_CODE (TREE_TYPE (op)) == REFERENCE_TYPE
	  && !CONSTANT_CLASS_P (op));
}

/* P3100: if PTR, stripped of conversions, is the address of an empty base
   class subobject of `*p' at a nonzero offset, which build_base_path forms
   as `p + offset' (such a base has no field to name), return p; otherwise
   NULL_TREE.  The address is never null, so a null check of it checks
   nothing; the indirection is through p.  An offset smaller than the class
   p points to cannot step to another element of an array.  */

tree
ubsan_p3100_base_offset_object (tree ptr)
{
  STRIP_NOPS (ptr);
  if (TREE_CODE (ptr) != POINTER_PLUS_EXPR
      || TREE_CODE (TREE_OPERAND (ptr, 1)) != INTEGER_CST
      || integer_zerop (TREE_OPERAND (ptr, 1)))
    return NULL_TREE;
  tree obj = TREE_OPERAND (ptr, 0);
  tree type = TREE_TYPE (TREE_TYPE (tree_strip_nop_conversions (obj)));
  if (!RECORD_OR_UNION_TYPE_P (type)
      || !COMPLETE_TYPE_P (type)
      || TREE_CODE (TYPE_SIZE_UNIT (type)) != INTEGER_CST
      || !tree_int_cst_lt (TREE_OPERAND (ptr, 1), TYPE_SIZE_UNIT (type)))
    return NULL_TREE;
  return obj;
}

/* P3100: if *OP_P, the pointer a reference is bound to or a member
   function is called on, is the address of an empty base class subobject
   of `*p' at a nonzero offset, `p + offset' (ubsan_p3100_base_offset_object),
   and the .indirection leaf of {expr.unary.dereference.nullptr} applies at
   LOC, rewrite *OP_P to evaluate p once and return the check that p is not
   null; otherwise NULL_TREE.  A p that is the address of an object, a
   member subobject (checked by the ubsan pass) or a reference already is
   not checked.  */

tree
ubsan_p3100_check_base_offset_object (location_t loc, tree *op_p)
{
  if (!flag_contracts_p3100 || current_function_decl == NULL_TREE)
    return NULL_TREE;
  tree obj = ubsan_p3100_base_offset_object (*op_p);
  if (obj == NULL_TREE)
    return NULL_TREE;
  tree sobj = tree_strip_nop_conversions (obj);
  if (TREE_CODE (sobj) == ADDR_EXPR
      || CONSTANT_CLASS_P (sobj)
      || ubsan_p3100_reference_glvalue_p (obj))
    return NULL_TREE;
  tree saved = save_expr (obj);
  tree check = ubsan_build_p3100_indirection_check (loc, saved);
  if (check == NULL_TREE)
    return NULL_TREE;
  /* Rebuild the address on the saved pointer.  */
  tree *p = op_p;
  while (TREE_CODE (*p) != POINTER_PLUS_EXPR)
    {
      *p = copy_node (*p);
      p = &TREE_OPERAND (*p, 0);
    }
  *p = copy_node (*p);
  TREE_OPERAND (*p, 0) = saved;
  return check;
}

/* P3100: OP stripped of conversions; if that is pointer arithmetic
   guarded against wrapping around the address space
   (build_implicit_pointer_wrap_check), `force, (wrapped ? <reaction / p>
   : p + i)', the sum `p + i' it computes when nothing wraps, stripped of
   conversions.  */

tree
ubsan_p3100_unguarded_sum (tree op)
{
  STRIP_NOPS (op);
  if (TREE_CODE (op) == COMPOUND_EXPR)
    {
      /* A conversion of the guarded sum, folded through the comma.  */
      tree guard = TREE_OPERAND (op, 1);
      STRIP_NOPS (guard);
      if (TREE_CODE (guard) == COND_EXPR)
	{
	  tree sum = TREE_OPERAND (guard, 2);
	  STRIP_NOPS (sum);
	  if (TREE_CODE (sum) == POINTER_PLUS_EXPR)
	    return sum;
	}
    }
  return op;
}

/* P3100: SUB being the address `p + c' of the element at a constant
   subscript (ubsan_p3100_constant_subscript), the pointer its null check
   checks: p, unless p is a pointer plus a constant itself
   (P3100_ARITH_ADDRESS_P) guarded against wrapping around the address
   space (build_implicit_pointer_wrap_check), `force, (wrapped ? <reaction
   / q> : q + c2)' with force `q, c2', saved -- a chain `q + 1 + 2' whose
   inner sum the guard keeps fold from combining with the outer one --
   then the innermost pointer of the chain, q (DECISIONS.md E103 (a)).  */

tree
ubsan_p3100_subscript_pointer (tree sub)
{
  tree ptr = TREE_OPERAND (sub, 0);
  while (true)
    {
      tree guarded = tree_strip_nop_conversions (ptr);
      if (TREE_CODE (guarded) == SAVE_EXPR)
	guarded = tree_strip_nop_conversions (TREE_OPERAND (guarded, 0));
      if (TREE_CODE (guarded) != COMPOUND_EXPR)
	return ptr;
      tree inner = ubsan_p3100_unguarded_sum (guarded);
      if (TREE_CODE (inner) != POINTER_PLUS_EXPR
	  || !P3100_ARITH_ADDRESS_P (inner)
	  || TREE_CODE (TREE_OPERAND (inner, 1)) != INTEGER_CST)
	return ptr;
      /* The guard's own shape: it evaluates q first.  */
      tree force = TREE_OPERAND (guarded, 0);
      if (TREE_CODE (force) != COMPOUND_EXPR)
	return ptr;
      tree first = TREE_OPERAND (force, 0);
      if (CONVERT_EXPR_P (first) && VOID_TYPE_P (TREE_TYPE (first)))
	first = TREE_OPERAND (first, 0);
      if (tree_strip_nop_conversions (first)
	  != tree_strip_nop_conversions (TREE_OPERAND (inner, 0)))
	return ptr;
      ptr = TREE_OPERAND (inner, 0);
    }
}

/* P3100: if OP, stripped of conversions, is the address `p + c' of the
   element at a constant subscript of the pointer p, p[c]
   (P3100_SUBSCRIPT_ADDRESS_P), return that POINTER_PLUS_EXPR; otherwise
   NULL_TREE.  A null check of the element checks p itself (DECISIONS.md
   E97 (c)): `p[c]' is `*(p + c)', so a null p is reported whatever c is,
   as its accesses are (the ubsan pass sees them as `MEM[p + c]',
   cp_gimplify_expr).  Also if OP is that sum guarded against wrapping
   around the address space (ubsan_p3100_unguarded_sum), p then a saved
   expression the guard evaluates.  Not if the offset would change the
   alignment ALIGN (in bytes, 0 for none) the check also tests, nor if p is
   the address of an object or a constant, which is not checked (the
   innermost pointer of a guarded chain, ubsan_p3100_subscript_pointer).  */

tree
ubsan_p3100_constant_subscript (tree op, unsigned HOST_WIDE_INT align)
{
  if (!flag_contracts_p3100 || op == NULL_TREE)
    return NULL_TREE;
  op = ubsan_p3100_unguarded_sum (op);
  if (TREE_CODE (op) != POINTER_PLUS_EXPR
      || !P3100_SUBSCRIPT_ADDRESS_P (op)
      || TREE_CODE (TREE_OPERAND (op, 1)) != INTEGER_CST)
    return NULL_TREE;
  if (align > 1
      && (TREE_INT_CST_LOW (TREE_OPERAND (op, 1)) & (align - 1)) != 0)
    return NULL_TREE;
  tree ptr = tree_strip_nop_conversions (ubsan_p3100_subscript_pointer (op));
  if (TREE_CODE (ptr) == SAVE_EXPR)
    ptr = tree_strip_nop_conversions (TREE_OPERAND (ptr, 0));
  if (TREE_CODE (ptr) == ADDR_EXPR || CONSTANT_CLASS_P (ptr))
    return NULL_TREE;
  return op;
}

/* P3100: if OP is the derived object's address of a downcast of `*p' to a
   reference (P3100_DOWNCAST_ADDRESS_P) that -fsanitize=vptr's check of the
   downcast wraps, `(<vptr check>, (D *) (p - offset))', possibly saved as
   a member call's object argument, return the downcast's sum
   `p - offset'; otherwise NULL_TREE.  Not if p has side effects, which a
   null check of p would evaluate again.  */

tree
ubsan_p3100_vptr_checked_downcast (tree op)
{
  if (!flag_contracts_p3100)
    return NULL_TREE;
  bool wrapped = false;
  while (true)
    {
      STRIP_NOPS (op);
      if (TREE_CODE (op) == SAVE_EXPR)
	op = TREE_OPERAND (op, 0);
      else if (TREE_CODE (op) == COMPOUND_EXPR)
	{
	  op = TREE_OPERAND (op, 1);
	  wrapped = true;
	}
      else
	break;
    }
  if (!wrapped
      || TREE_CODE (op) != POINTER_PLUS_EXPR
      || !P3100_DOWNCAST_ADDRESS_P (op)
      || TREE_SIDE_EFFECTS (TREE_OPERAND (op, 0)))
    return NULL_TREE;
  return op;
}

/* P3100: true if OP is the address of the element at a constant subscript
   (ubsan_p3100_constant_subscript) guarded against wrapping around the
   address space (ubsan_p3100_unguarded_sum).  */

bool
ubsan_p3100_guarded_subscript_p (tree op)
{
  tree sub = ubsan_p3100_constant_subscript (op, 0);
  if (sub == NULL_TREE)
    return false;
  STRIP_NOPS (op);
  return op != sub;
}

/* P3100: SUB being the address `p + c' of the element at a constant
   subscript (ubsan_p3100_constant_subscript) found in OP, return it formed
   from PTR, p evaluated once, instead.  If OP guards SUB against wrapping
   around the address space, return OP itself: its p is a saved expression
   that PTR evaluated, and the guard stays.  */

tree
ubsan_p3100_rebuild_subscript (tree sub, tree ptr, tree op)
{
  tree stripped = op;
  STRIP_NOPS (stripped);
  if (stripped != sub)
    return op;
  tree r = build2 (POINTER_PLUS_EXPR, TREE_TYPE (sub), ptr,
		   TREE_OPERAND (sub, 1));
  P3100_SUBSCRIPT_ADDRESS_P (r) = 1;
  P3100_DOWNCAST_ADDRESS_P (r) = P3100_DOWNCAST_ADDRESS_P (sub);
  return r;
}

static tree ubsan_maybe_instrument_reference_or_call (location_t, tree, tree,
						      enum ubsan_null_ckind,
						      bool = false);

/* P3100: if OP, the object pointer of OP->*PM, is the address of a member
   subobject, (p->s).*pm, check the pointer the object is reached through,
   p, instead: that is the dereference, and the ubsan pass then checks
   neither the address of the member nor the addresses formed from it
   (ubsan_ptrmem_derived_p), so it is reported once, and for a member at a
   nonzero offset too.  Return true if OP is such an address, with *RESULT
   the address formed from the checked pointer, or NULL_TREE if it needs no
   check.  */

static bool
ubsan_p3100_instrument_ptrmem_subobject (location_t loc, tree op,
					 tree *result)
{
  if (!flag_contracts_p3100
      || TREE_CODE (op) != ADDR_EXPR
      || !handled_component_p (TREE_OPERAND (op, 0)))
    return false;
  tree base = TREE_OPERAND (op, 0);
  while (handled_component_p (base))
    base = TREE_OPERAND (base, 0);
  if (!INDIRECT_REF_P (base) && TREE_CODE (base) != MEM_REF)
    return false;
  tree ptr = TREE_OPERAND (base, 0);
  tree stripped = tree_strip_nop_conversions (ptr);
  if (!POINTER_TYPE_P (TREE_TYPE (ptr))
      || TREE_CODE (stripped) == ADDR_EXPR
      || CONSTANT_CLASS_P (stripped))
    return false;
  tree r = ubsan_maybe_instrument_reference_or_call (loc, ptr, TREE_TYPE (ptr),
						     UBSAN_MEMBER_ACCESS);
  if (!r)
    {
      *result = NULL_TREE;
      return true;
    }
  /* Rebuild the address on the checked pointer.  */
  op = copy_node (op);
  TREE_SIDE_EFFECTS (op) = 1;
  tree *slot = &TREE_OPERAND (op, 0);
  while (handled_component_p (*slot))
    {
      *slot = copy_node (*slot);
      TREE_SIDE_EFFECTS (*slot) = 1;
      slot = &TREE_OPERAND (*slot, 0);
    }
  *slot = copy_node (*slot);
  TREE_SIDE_EFFECTS (*slot) = 1;
  TREE_OPERAND (*slot, 0) = r;
  *result = op;
  return true;
}

static tree
ubsan_maybe_instrument_reference_or_call (location_t loc, tree op, tree ptype,
					  enum ubsan_null_ckind ckind,
					  bool p3100_access)
{
  if (current_function_decl == NULL_TREE)
    return NULL_TREE;

  tree subobject;
  if (ckind == UBSAN_MEMBER_ACCESS
      && ubsan_p3100_instrument_ptrmem_subobject (loc, op, &subobject))
    return subobject;

  /* P3100: a reference bound to, or a member function called on, an empty
     base class subobject at a nonzero offset has its object pointer
     checked (ubsan_p3100_check_base_offset_object), first; the address
     itself is never null.  */
  tree base_check = NULL_TREE;
  if (ckind == UBSAN_REF_BINDING || ckind == UBSAN_MEMBER_CALL)
    base_check = ubsan_p3100_check_base_offset_object (loc, &op);
  tree base_op = op;

  /* P3100 routes null-dereference and misalignment to the contract-violation
     machinery.  This gate covers two of the three syntactic shapes of that UB
     -- a reference binding (int &r = *p) and a null-this member call
     (p->f ()); the third, a plain *p, is handled by the sibling gates.  All
     three must consult the routed reaction, or a configured check applies to
     some spellings of one dereference and not others.  None of the shapes
     gated here accesses the object, so a null pointer is reported as the
     .indirection leaf of the group; the alignment check is that group's
     .store leaf, as at every access.  The exception is the object pointer
     of OP->*PM (UBSAN_MEMBER_ACCESS), whose member may yet be read or
     written: that is not known until the expression is complete, so it is
     instrumented when either leaf applies, built as .indirection, and
     retargeted to .access at genericization when the member is accessed
     (cp_ubsan_retarget_ptrmem_object_checks).  So is the pointer a
     reference is bound to, for an access through the reference within the
     expression, which the ubsan pass reports by retargeting the binding's
     check to .access (ubsan_binding_check_for): with .access alone, the
     check is built with no reaction.

     Resolve both reactions up front so they can widen the gate below.
     If P3100_ACCESS, the null check is the .access leaf instead: a member
     call of a virtual function by its unqualified name that the front end
     resolved statically (DECISIONS.md E110 (a)).  */
  const char *p3100_null_group
    = (p3100_access
       ? "ub:expr.unary.dereference.nullptr.access"
       : "ub:expr.unary.dereference.nullptr.indirection");
  int p3100_null_reaction
    = resolve_implicit_ub_reaction (current_function_decl, loc,
				    p3100_null_group);
  int p3100_align_reaction
    = resolve_implicit_ub_reaction (current_function_decl, loc,
				    "ub:basic.align.object.alignment.store");
  bool p3100_null = (p3100_null_reaction != IMPLICIT_UB_NONE
		     || ((ckind == UBSAN_MEMBER_ACCESS
			  || ckind == UBSAN_REF_BINDING)
			 && (resolve_implicit_ub_reaction
			     (current_function_decl, loc,
			      "ub:expr.unary.dereference.nullptr.access")
			     != IMPLICIT_UB_NONE)));
  /* A reference bound to, or a member function called on, a member
     subobject has its null dereference checked on the object pointer, and
     reported once.  */
  bool checked_subobject
    = (flag_contracts_p3100
       && (ckind == UBSAN_REF_BINDING || ckind == UBSAN_MEMBER_CALL)
       && ubsan_p3100_checked_subobject_p (op));
  if (p3100_null && checked_subobject)
    {
      p3100_null_reaction = IMPLICIT_UB_NONE;
      p3100_null = false;
    }
  /* A reference bound to, or a member function called on, a glvalue that
     is a reference already -- a reference variable or parameter converted
     (`const T &r2 = r', the `static_cast<T &&> (t)' of std::move), to a
     base class too, a call returning a reference, `r.f ()' -- indirects
     through no pointer here: the object was reached where that reference
     was bound, and checked there (ubsan_p3100_reference_glvalue_p).  And
     the address of an empty base class subobject at a nonzero offset is
     never null: its object pointer is checked instead
     (ubsan_p3100_check_base_offset_object).  */
  if (p3100_null
      && (ckind == UBSAN_REF_BINDING || ckind == UBSAN_MEMBER_CALL)
      && (ubsan_p3100_reference_glvalue_p (op)
	  || ubsan_p3100_base_offset_object (op)))
    {
      p3100_null_reaction = IMPLICIT_UB_NONE;
      p3100_null = false;
    }
  /* Nor does the object of OP->*PM that is a reference already, `r.*pm'
     (DECISIONS.md E92 (a)).  */
  if (p3100_null
      && ckind == UBSAN_MEMBER_ACCESS
      && ubsan_p3100_reference_glvalue_p (op))
    {
      p3100_null_reaction = IMPLICIT_UB_NONE;
      p3100_null = false;
    }
  bool p3100_align = (p3100_align_reaction != IMPLICIT_UB_NONE);

  if (!sanitize_flags_p (SANITIZE_ALIGNMENT | SANITIZE_NULL)
      && !p3100_null && !p3100_align)
    return (base_check
	    ? build2 (COMPOUND_EXPR, TREE_TYPE (base_op), base_check, base_op)
	    : NULL_TREE);

  tree type = TREE_TYPE (ptype);
  tree orig_op = op;
  bool instrument = false;
  unsigned int mina = 0;

  if (sanitize_flags_p (SANITIZE_ALIGNMENT) || p3100_align)
    {
      mina = min_align_of_type (type);
      if (mina <= 1)
	mina = 0;
    }
  /* Under -fcontracts-p3100, nor is -fsanitize=null's check of such a
     subobject's address made: the object pointer's check, P3100's or the
     sanitizer's, reports the null dereference once (DECISIONS.md E120).
     With an alignment edge the check is made as it is.  */
  if (checked_subobject && !mina)
    return (base_check
	    ? build2 (COMPOUND_EXPR, TREE_TYPE (base_op), base_check, base_op)
	    : NULL_TREE);
  while ((TREE_CODE (op) == NOP_EXPR
	  || TREE_CODE (op) == NON_LVALUE_EXPR)
	 && TREE_CODE (TREE_TYPE (op)) == POINTER_TYPE)
    op = TREE_OPERAND (op, 0);
  if (TREE_CODE (op) == NOP_EXPR
      && TREE_CODE (TREE_TYPE (op)) == REFERENCE_TYPE)
    {
      if (mina && mina > min_align_of_type (TREE_TYPE (TREE_TYPE (op))))
	instrument = true;
    }
  else
    {
      if ((sanitize_flags_p (SANITIZE_NULL) || p3100_null)
	  && TREE_CODE (op) == ADDR_EXPR)
	{
	  /* tree_single_nonzero_p will not return true for non-weak
	     non-automatic decls with -fno-delete-null-pointer-checks,
	     which is disabled during -fsanitize=null.  We don't want to
	     instrument those, just weak vars though.  */
	  int save_flag_delete_null_pointer_checks
	    = flag_delete_null_pointer_checks;
	  flag_delete_null_pointer_checks = 1;
	  if (!tree_single_nonzero_p (op))
	    instrument = true;
	  flag_delete_null_pointer_checks
	    = save_flag_delete_null_pointer_checks;
	}
      else if (sanitize_flags_p (SANITIZE_NULL) || p3100_null)
	instrument = true;
      if (mina && mina > 1)
	{
	  if (!POINTER_TYPE_P (TREE_TYPE (op))
	      || mina > get_pointer_alignment (op) / BITS_PER_UNIT)
	    instrument = true;
	}
    }
  if (!instrument)
    return (base_check
	    ? build2 (COMPOUND_EXPR, TREE_TYPE (base_op), base_check, base_op)
	    : NULL_TREE);
  /* P3100: the element at a constant subscript of a pointer, `S &r =
     sp[1]', `sp[1].f ()', is checked on the pointer itself, as its
     accesses are (DECISIONS.md E97 (c)), unless the offset changes the
     alignment tested; the address is then formed from the checked
     pointer.  So is the derived object of a downcast of `*p' to a
     reference (P3100_DOWNCAST_ADDRESS_P), by -fsanitize=null's check too
     where no P3100 null check is made: that address is not null where p
     is, and the conversion made no null test.  */
  tree sub = NULL_TREE;
  tree sub_op = NULL_TREE;
  if (p3100_null || sanitize_flags_p (SANITIZE_NULL))
    {
      sub = ubsan_p3100_constant_subscript (orig_op, mina);
      if (sub && !p3100_null && !P3100_DOWNCAST_ADDRESS_P (sub))
	sub = NULL_TREE;
      /* A member call's object argument, the downcast wrapped by its vptr
	 check and saved: check p, and leave the argument as it is
	 (ubsan_p3100_rebuild_subscript returns it).  */
      if (!sub && ckind == UBSAN_MEMBER_CALL)
	{
	  sub = ubsan_p3100_vptr_checked_downcast (orig_op);
	  if (sub
	      && mina > 1
	      && (TREE_INT_CST_LOW (TREE_OPERAND (sub, 1)) & (mina - 1)) != 0)
	    sub = NULL_TREE;
	}
      if (sub)
	{
	  sub_op = orig_op;
	  orig_op = ubsan_p3100_subscript_pointer (sub);
	}
    }
  op = save_expr (orig_op);
  tree value = op;
  /* The object pointer of OP->*PM gets a temporary of its own even when it
     could be evaluated again (`this', a parameter declared const): the
     ubsan pass recognizes the addresses formed from it by that temporary,
     and does not check them again (ubsan_ptrmem_derived_p).  */
  if (ckind == UBSAN_MEMBER_ACCESS
      && TREE_CODE (op) != SAVE_EXPR
      && !CONSTANT_CLASS_P (op))
    {
      op = build1_loc (EXPR_LOCATION (orig_op), SAVE_EXPR, TREE_TYPE (orig_op),
		       orig_op);
      TREE_SIDE_EFFECTS (op) = 1;
      value = op;
    }
  /* So does the pointer a reference is bound to under a P3100 null or
     alignment check, one named UBSAN_P3100_BOUND_NAME: the ubsan pass
     reports a read or write through the reference within the expression,
     the same null dereference, by this check, under the .access leaf
     (ubsan_binding_check_for), and the same misaligned access by its
     alignment check (ubsan_binding_checks_align_p; GCC-879).  */
  else if (ckind == UBSAN_REF_BINDING
	   && (p3100_null
	       || (mina && p3100_align_reaction != IMPLICIT_UB_NONE))
	   && !TREE_CONSTANT (orig_op))
    {
      tree var = build_decl (loc, VAR_DECL,
			     get_identifier (UBSAN_P3100_BOUND_NAME),
			     TREE_TYPE (orig_op));
      DECL_ARTIFICIAL (var) = 1;
      DECL_IGNORED_P (var) = 1;
      DECL_CONTEXT (var) = current_function_decl;
      op = build4 (TARGET_EXPR, TREE_TYPE (orig_op), var, orig_op, NULL_TREE,
		   NULL_TREE);
      TREE_SIDE_EFFECTS (op) = 1;
      value = var;
    }
  gcc_assert (POINTER_TYPE_P (ptype));
  if (TREE_CODE (ptype) == REFERENCE_TYPE)
    ptype = build_pointer_type (TREE_TYPE (ptype));
  tree kind = build_int_cst (ptype, ckind);
  tree align = build_int_cst (pointer_sized_int_node, mina);
  /* UBSAN_NULL_REACTION is the P3100 implicit-UB null reaction,
     UBSAN_NULL_ENTRY/UBSAN_NULL_DATA the null contract handler entry point and
     its static data-block address; the UBSAN_NULL_ALIGN_* operands the same
     for alignment.  Carry the resolved reactions
     and handlers, mirroring instrument_mem_ref -- the reaction has to
     travel as an operand rather than be re-derived later, because by
     expansion time cfun->decl may be an inlined-into function with a
     different configuration.  */
  tree reaction = build_int_cst (unsigned_type_node, p3100_null_reaction);
  /* Only an access with an alignment edge (MINA != 0) has an alignment
     check to react to; carry NONE otherwise, as instrument_mem_ref does.  */
  if (!mina)
    p3100_align_reaction = IMPLICIT_UB_NONE;
  tree align_reaction
    = build_int_cst (unsigned_type_node, p3100_align_reaction);
  tree entryt, data_addrt;
  implicit_ub_handler_operands (current_function_decl, loc,
				p3100_null_group,
				p3100_null_reaction, &entryt, &data_addrt);
  tree align_entryt, align_data_addrt;
  implicit_ub_handler_operands (current_function_decl, loc,
				"ub:basic.align.object.alignment.store",
				p3100_align_reaction, &align_entryt,
				&align_data_addrt);
  tree call
    = build_call_expr_internal_loc (loc, IFN_UBSAN_NULL, void_type_node,
				    UBSAN_NULL_NUM_OPS, op, kind, align,
				    reaction, entryt, data_addrt,
				    align_reaction, align_entryt,
				    align_data_addrt);
  TREE_SIDE_EFFECTS (call) = 1;
  if (sub)
    value = fold_convert (TREE_TYPE (sub_op),
			  ubsan_p3100_rebuild_subscript (sub, value, sub_op));
  tree res = fold_build2 (COMPOUND_EXPR, TREE_TYPE (value), call, value);
  if (base_check)
    res = build2 (COMPOUND_EXPR, TREE_TYPE (res), base_check, res);
  return res;
}

/* P3100: build the check that OP, a single-evaluation pointer (a SAVE_EXPR
   or a decl), is not null, for an indirection through it whose result is
   not accessed -- `&*p', a discarded `*p', the object expression of a
   static member -- the .indirection leaf of
   {expr.unary.dereference.nullptr}; or, if ACCESS, for one that only reads
   through it what the front end then drops (the base offset a discarded
   conversion to a virtual base reads), the .access leaf.  Return the
   IFN_UBSAN_NULL call, which evaluates OP, or NULL_TREE if that leaf
   resolves to no check here.  -fsanitize=null checks none of these, so
   nothing is built for it.  */

tree
ubsan_build_p3100_indirection_check (location_t loc, tree op, bool access)
{
  if (!flag_contracts_p3100 || current_function_decl == NULL_TREE)
    return NULL_TREE;
  const char *group = (access
		       ? "ub:expr.unary.dereference.nullptr.access"
		       : "ub:expr.unary.dereference.nullptr.indirection");
  int reaction = resolve_implicit_ub_reaction (current_function_decl, loc,
					       group);
  if (reaction == IMPLICIT_UB_NONE)
    return NULL_TREE;
  tree ptype = TREE_TYPE (op);
  gcc_assert (POINTER_TYPE_P (ptype));
  tree entryt, data_addrt;
  implicit_ub_handler_operands (current_function_decl, loc, group, reaction,
				&entryt, &data_addrt);
  /* No alignment check: nothing is accessed.  */
  tree call
    = build_call_expr_internal_loc (loc, IFN_UBSAN_NULL, void_type_node,
				    UBSAN_NULL_NUM_OPS, op,
				    build_int_cst (ptype, (access
							   ? UBSAN_LOAD_OF
							   : UBSAN_REF_BINDING)),
				    build_int_cst (pointer_sized_int_node, 0),
				    build_int_cst (unsigned_type_node,
						   reaction),
				    entryt, data_addrt,
				    build_int_cst (unsigned_type_node,
						   IMPLICIT_UB_NONE),
				    null_pointer_node, null_pointer_node);
  TREE_SIDE_EFFECTS (call) = 1;
  return call;
}

/* Instrument a NOP_EXPR to REFERENCE_TYPE or INTEGER_CST with REFERENCE_TYPE
   type if needed.  P3100: a binding with no location of its own -- the
   conversion of an implicit reference binding, `int &r = *p', `f (*p)', a
   reference constant -- is checked at LOC, the location of the innermost
   expression around it that has one, which configuration matching and the
   contract_violation's location() use; otherwise the violation would have
   an empty file name and line 0.  */

void
ubsan_maybe_instrument_reference (tree *stmt_p, location_t loc)
{
  tree stmt = *stmt_p;
  tree op = stmt;
  if (TREE_CODE (stmt) == NOP_EXPR)
    op = TREE_OPERAND (stmt, 0);
  if (EXPR_HAS_LOCATION (stmt) || !flag_contracts_p3100)
    loc = EXPR_LOCATION (stmt);
  op = ubsan_maybe_instrument_reference_or_call (loc, op, TREE_TYPE (stmt),
						 UBSAN_REF_BINDING);
  if (op)
    {
      if (TREE_CODE (stmt) == NOP_EXPR)
	TREE_OPERAND (stmt, 0) = op;
      else
	*stmt_p = op;
    }
}

/* Instrument OP, the object pointer of a pointer-to-data-member access
   (OP->*PM), if needed, before the member's offset is added to it.  Return
   the instrumented pointer, or OP.  */

tree
ubsan_maybe_instrument_ptrmem_object (location_t loc, tree op)
{
  if (op == error_mark_node || !POINTER_TYPE_P (TREE_TYPE (op)))
    return op;
  tree r = ubsan_maybe_instrument_reference_or_call (loc, op, TREE_TYPE (op),
						     UBSAN_MEMBER_ACCESS);
  return r ? r : op;
}

/* Instrument a CALL_EXPR to a method if needed.  */

/* P3100: instrument OP, the object pointer of a member function called
   through a pointer to member function, before the `this' adjustment, as
   a member call's object pointer (cp_ubsan_maybe_instrument_ptrmemfunc_call);
   or, if ACCESS, the object pointer of a virtual function called by its
   unqualified name that the front end resolved statically, under the
   .access leaf (cp_ubsan_maybe_check_devirtualized_call).  Return the
   instrumented pointer, or NULL_TREE if it needs no check.  */

tree
ubsan_maybe_instrument_member_call_object (location_t loc, tree op,
					   bool access)
{
  return ubsan_maybe_instrument_reference_or_call (loc, op, TREE_TYPE (op),
						   UBSAN_MEMBER_CALL, access);
}

void
ubsan_maybe_instrument_member_call (tree stmt, bool is_ctor)
{
  if (call_expr_nargs (stmt) == 0)
    return;
  tree op = CALL_EXPR_ARG (stmt, 0);
  if (op == error_mark_node
      || !POINTER_TYPE_P (TREE_TYPE (op)))
    return;
  op = ubsan_maybe_instrument_reference_or_call (EXPR_LOCATION (stmt), op,
						 TREE_TYPE (op),
						 is_ctor ? UBSAN_CTOR_CALL
						 : UBSAN_MEMBER_CALL);
  if (op)
    CALL_EXPR_ARG (stmt, 0) = op;
}

/* P3100 ASan contract routing.

   Under -fcontracts-p3100 the resolved contract-evaluation semantic for the
   user-space address check (SANITIZE_USER_ADDRESS) controls how a detected
   AddressSanitizer error is handled at run time:

     observe -> the runtime reports the error via the contract-violation
                handler and the program continues;
     enforce -> the runtime reports and then terminates.

   P3100: because the routed report is invoked from inside libasan's
   implicitly-noexcept ScopedInErrorReport destructor, a throwing handler can
   never propagate -- it hits "exception escaping a noexcept function" and
   std::terminate()s (see libsanitizer/asan/asan_report.cpp for the routing
   details).  The address check therefore offers only NON-throwing
   realizations, and its resolved semantic is one of:

     assume         -> realized per function via no_sanitize
                       (c_apply_contract_sanitizer_assume);
                       no descriptor emitted here;
     quick_enforce  -> terminate on the error WITHOUT calling the handler;
                       always available (default when -fcontracts-p4298 is off);
     noexcept_observe (P4298) -> call the handler, then continue;
     noexcept_enforce (P4298) -> call the handler, then terminate.

   noexcept_observe/noexcept_enforce require -fcontracts-p4298 and are the
   run-time continue/terminate-after-handler paths; plain throwing
   enforce/observe and ignore are hard-errored during option processing, so
   they never reach here.

   The AddressSanitizer instrumentation itself is semantic-INDEPENDENT: every
   realization emits identical __asan_report_* calls, and the run-time behavior
   is decided by the libsanitizer report path reading the descriptor emitted
   here.  So we convey the resolved semantic to libsanitizer with a per-TU weak
   byte __asan_contract_semantic whose wire encoding is decoupled from the
   internal contract_evaluation_semantic values:

     0 = stock  (symbol absent / routing off -> runtime keeps stock behavior)
     1 = observe        (noexcept_observe: call handler, then continue)
     2 = enforce        (noexcept_enforce: call handler, then terminate)
     3 = quick_enforce  (terminate WITHOUT calling the handler)

   Every translation unit emits its own, but being weak they resolve to one
   definition program-wide, and which translation unit's survives is
   unspecified: a program whose translation units were compiled with
   different routing flags gets one of their semantics for all of them.

   Crucially this descriptor is emitted here in the per-TU FRONT-END compile,
   where -fcontracts-p3100 and the -fsanitize-semantic/-recover/-trap flags are
   live.  It is a preserved compile-time global, so under -flto it streams
   through WPA/LTRANS into the final image exactly like a user global -- the LTO
   and non-LTO paths become identical and no LTO-time flag state is needed.
   (Emitting it from asan_finish_file, which runs at LTRANS, would read stale
   flag state and silently drop routing under -flto.)

   -fsanitize-noncontract-callbacks is the global opt-out: when set,
   we emit no descriptor, so the runtime reads stock (0) and takes its stock
   report path -- and, because the runtime guardrail keys off the
   same descriptor, the guardrail disengages too.  The opt-out is orthogonal to
   the instrument/allowed-set decisions: assume is still realized per function
   via no_sanitize (c_apply_contract_sanitizer_assume), and the
   -fsanitize-semantic= allowed-set validation still applies during option
   processing.  It affects only the runtime report routing and the
   guardrail.  */

/* The value of a routing wire byte, as libsanitizer reads it (ContractRoute
   in sanitizer_common/sanitizer_contract_routing.h).  */
enum contract_route
{
  CONTRACT_ROUTE_STOCK = 0,	/* routing off: stock sanitizer behavior */
  CONTRACT_ROUTE_OBSERVE = 1,	/* call the handler, then continue */
  CONTRACT_ROUTE_ENFORCE = 2,	/* call the handler, then terminate */
  CONTRACT_ROUTE_QUICK = 3	/* terminate without calling the handler */
};

/* The wire value for a routed check whose semantic resolved to SEM.  assume
   is realized via per-function no_sanitize; ignore and plain throwing
   enforce/observe are hard-errored; CES_INVALID means routing is off (or the
   doomed recover-without-p4298 compile).  All of those leave the runtime its
   stock behavior.  */

static contract_route
contract_route_for (contract_evaluation_semantic sem)
{
  switch (sem)
    {
    case CES_NOEXCEPT_OBSERVE:
      return CONTRACT_ROUTE_OBSERVE;
    case CES_NOEXCEPT_ENFORCE:
      return CONTRACT_ROUTE_ENFORCE;
    case CES_QUICK:
      return CONTRACT_ROUTE_QUICK;
    default:
      return CONTRACT_ROUTE_STOCK;
    }
}

/* Emit one weak wire byte NAME conveying the resolved routing semantic of the
   check CHECK_BIT to libsanitizer, or emit nothing when the check is not
   routed to the handler.  The ASan address check and the two pointer-pair
   checks each get their own byte so the address routing scope is independent
   of the pointer-pair checks (see libsanitizer/asan/asan_report.cpp), and TSan
   has one of its own.  */

static void
emit_contract_wire_byte (const char *name, sanitize_code_type check_bit)
{
  contract_route wire
    = contract_route_for (resolved_sanitizer_semantic (check_bit));
  if (wire == CONTRACT_ROUTE_STOCK)
    return;

  tree id = get_identifier (name);
  tree var = build_decl (UNKNOWN_LOCATION, VAR_DECL, id,
			 unsigned_char_type_node);
  TREE_STATIC (var) = 1;
  TREE_PUBLIC (var) = 1;
  DECL_WEAK (var) = 1;
  DECL_ARTIFICIAL (var) = 1;
  /* The descriptor has no in-TU references -- only the sanitizer runtime reads
     it -- so mark it preserved.  Otherwise (whole-program) analysis would
     eliminate it as an unreferenced symbol and routing would silently fall
     back to stock behavior, in particular under -flto.  */
  DECL_PRESERVE_P (var) = 1;
  DECL_INITIAL (var) = build_int_cst (unsigned_char_type_node, wire);
  varpool_node::finalize_decl (var);
}

/* AddressSanitizer routing descriptor: the weak wire byte
   __asan_contract_semantic for the address checks, and one each for the two
   pointer-pair checks, read by libasan's ScopedInErrorReport.  Nothing is
   emitted without -fcontracts-p3100, or when the sanitizer's own callbacks
   are in use.  */

static void
emit_asan_contract_semantic_descriptor (void)
{
  if (!flag_contracts_p3100 || flag_sanitize_noncontract_callbacks)
    return;

  emit_contract_wire_byte ("__asan_contract_semantic",
				SANITIZE_USER_ADDRESS);
  /* The two ASan pointer-pair checks report through the same
     ScopedInErrorReport path but are governed by their own bytes, so the
     address byte's scope is unchanged.  */
  emit_contract_wire_byte ("__asan_contract_semantic_pointer_compare",
				SANITIZE_POINTER_COMPARE);
  emit_contract_wire_byte ("__asan_contract_semantic_pointer_subtract",
				SANITIZE_POINTER_SUBTRACT);
}

/* ThreadSanitizer routing descriptor: a single whole-program check (data race)
   with its own weak wire byte __tsan_contract_semantic, read by libtsan's
   OutputReport.  Same wire encoding and emit-nothing-for-stock rule as the ASan
   descriptor; reuses the shared wire-byte emitter.  */

static void
emit_tsan_contract_semantic_descriptor (void)
{
  if (!flag_contracts_p3100 || flag_sanitize_noncontract_callbacks)
    return;

  emit_contract_wire_byte ("__tsan_contract_semantic", SANITIZE_THREAD);
}

/* P3100 (UBSan runtime routing): the per-check analog of the ASan
   descriptor above.  Whereas ASan has a single whole-program check (one byte),
   UBSan has many independently-configurable runtime checks, so the conveyance
   is a weak ARRAY, __ubsan_contract_semantic[RUC_COUNT], one wire byte per
   routed check.  libubsan's ScopedReport destructor folds its ErrorType to a
   routed id and reads the corresponding byte (see
   libsanitizer/ubsan/ubsan_diag.cpp).

   The routed-check id ordering must stay in sync with the RUC_* enum in
   ubsan_diag.cpp.  Adding a routed check now touches TWO locations:
     1. gcc/contracts-routed-checks.def -- which generates this RUC_* enum,
        routed_checks[] below, c_apply_contract_sanitizer_assume's list and the
        UBSan half of opts.cc's ROUTED_SANITIZER_BITS;
     2. the matching RUC_* id + ErrorTypeToRoutedId case on the runtime side
        (libsanitizer/ubsan/ubsan_diag.cpp), which is a separate build unit and
        cannot share the header -- so that one still has to be kept in step by
        hand, and the ordering comment in the .def says so.

   Generating the three compiler-side copies from the .def rather than writing
   each out by hand is what keeps that count at two.  Two of the three fail
   SILENTLY when they disagree: a check missing from the recognition mask is
   never routed, and one missing from the assume list never suppresses its
   instrumentation, neither with any diagnostic.

   Same wire encoding and the same emit-nothing-for-stock rule as the ASan
   descriptor.  The Clang-only ids (RUC_FUNCTION and the four at the end) are
   DEF_ROUTED_UBSAN_RESERVED rows: they hold a wire slot for id alignment, GCC
   has no bit for them, so they contribute no routed_checks[] row and their
   wire byte always stays stock.  */

enum
{
#define DEF_ROUTED_UBSAN_CHECK(ID, BIT) ID,
#define DEF_ROUTED_UBSAN_RESERVED(ID) ID,
#include "contracts-routed-checks.def"
  RUC_COUNT
};

static void
emit_ubsan_contract_semantic_descriptor (void)
{
  if (!flag_contracts_p3100 || flag_sanitize_noncontract_callbacks)
    return;

  /* Routed UBSan runtime checks: { RUC id, its -fsanitize= bit }.  Generated
     from contracts-routed-checks.def; the reserved Clang-only slots have no
     GCC bit and so contribute no row.  */
  static const struct { int id; sanitize_code_type bit; }
  routed_checks[] = {
#define DEF_ROUTED_UBSAN_CHECK(ID, BIT) { ID, BIT },
#include "contracts-routed-checks.def"
  };

  unsigned char wire[RUC_COUNT];
  memset (wire, CONTRACT_ROUTE_STOCK, sizeof (wire));
  bool any_routed = false;

  for (unsigned i = 0; i < ARRAY_SIZE (routed_checks); ++i)
    {
      contract_route w = contract_route_for
	(resolved_sanitizer_semantic (routed_checks[i].bit));
      wire[routed_checks[i].id] = w;
      if (w != CONTRACT_ROUTE_STOCK)
	any_routed = true;
    }

  /* No routed check resolves to a routing semantic -- emit nothing so the
     runtime keeps stock behavior.  */
  if (!any_routed)
    return;

  tree elt_type = unsigned_char_type_node;
  tree array_type
    = build_array_type (elt_type, build_index_type (size_int (RUC_COUNT - 1)));
  tree name = get_identifier ("__ubsan_contract_semantic");
  tree var = build_decl (UNKNOWN_LOCATION, VAR_DECL, name, array_type);
  TREE_STATIC (var) = 1;
  TREE_PUBLIC (var) = 1;
  DECL_WEAK (var) = 1;
  DECL_ARTIFICIAL (var) = 1;
  /* Only the sanitizer runtime reads it; mark preserved so whole-program
     analysis (in particular -flto) does not eliminate it as unreferenced.  */
  DECL_PRESERVE_P (var) = 1;

  vec<constructor_elt, va_gc> *elts = NULL;
  for (int i = 0; i < RUC_COUNT; ++i)
    CONSTRUCTOR_APPEND_ELT (elts, size_int (i),
			    build_int_cst (elt_type, wire[i]));
  tree ctor = build_constructor (array_type, elts);
  TREE_STATIC (ctor) = 1;
  TREE_CONSTANT (ctor) = 1;
  DECL_INITIAL (var) = ctor;
  varpool_node::finalize_decl (var);
}

/* Emit the P3100 sanitizer contract-routing descriptors of this translation
   unit: ASan's, UBSan's and TSan's.  Called at the end of the translation
   unit by the C and C++ front ends, while -fcontracts-p3100 and the
   -fsanitize-semantic= flags are live.  */

void
c_emit_contract_sanitizer_descriptors (void)
{
  emit_asan_contract_semantic_descriptor ();
  emit_ubsan_contract_semantic_descriptor ();
  emit_tsan_contract_semantic_descriptor ();
}

/* P3100 (assume): mark FNDECL no_sanitize for each routed sanitizer check
   that resolves to assume, so that it is not instrumented -- byte identical
   to a build without that -fsanitize= check.  The attribute lives in
   DECL_ATTRIBUTES and so streams through LTO.  Called by the C and C++ front
   ends before genericization, whose own instrumentation consults it.  */

void
c_apply_contract_sanitizer_assume (tree fndecl)
{
  /* P3100 (assume): when -fcontracts-p3100 resolves the user-space
     address check to assume, AddressSanitizer must not instrument -- byte
     identical to a build without -fsanitize=address for that check.  Realize
     this per function by marking the definition no_sanitize("address").  Unlike
     gating the asan pass on the front-end flags (which are gone at LTRANS), the
     no_sanitize attribute lives in DECL_ATTRIBUTES and streams through LTO, so
     pass_asan honors it under both -flto and non-LTO.  */
  if (flag_contracts_p3100
      && (flag_sanitize & SANITIZE_ADDRESS)
      && resolved_sanitizer_semantic (SANITIZE_USER_ADDRESS) == CES_ASSUME)
    add_no_sanitize_value (fndecl, SANITIZE_ADDRESS);

  /* Same for the two ASan pointer-pair checks (pointer-compare,
     pointer-subtract): when one resolves to assume, its instrumentation
     (__sanitizer_ptr_cmp / __sanitizer_ptr_sub) must not be emitted.  Each is a
     dedicated ASan-family bit, so mark no_sanitize for it individually.  */
  if (flag_contracts_p3100)
    {
      static const sanitize_code_type pointer_pair_bits[]
	= { SANITIZE_POINTER_COMPARE, SANITIZE_POINTER_SUBTRACT };
      for (unsigned i = 0; i < ARRAY_SIZE (pointer_pair_bits); ++i)
	if ((flag_sanitize & pointer_pair_bits[i])
	    && (resolved_sanitizer_semantic (pointer_pair_bits[i])
		== CES_ASSUME))
	  add_no_sanitize_value (fndecl, pointer_pair_bits[i]);
    }

  /* Same for the routed UBSan runtime checks (vptr, alignment, object-size,
     nonnull-attribute, returns-nonnull-attribute, pointer-overflow): when one
     resolves to assume, its instrumentation must not be emitted (byte identical
     to a build without that -fsanitize= check), realized per function via
     no_sanitize so it streams through LTO.  */
  if (flag_contracts_p3100)
    {
      /* Generated from contracts-routed-checks.def -- the same list the
	 wire descriptor is built from, so a check cannot be routed without
	 also being realizable as "assume" here.  */
      static const sanitize_code_type routed_ubsan_bits[] = {
#define DEF_ROUTED_UBSAN_CHECK(ID, BIT) BIT,
#include "contracts-routed-checks.def"
      };
      for (unsigned i = 0; i < ARRAY_SIZE (routed_ubsan_bits); ++i)
	if ((flag_sanitize & routed_ubsan_bits[i])
	    && (resolved_sanitizer_semantic (routed_ubsan_bits[i])
		== CES_ASSUME))
	  add_no_sanitize_value (fndecl, routed_ubsan_bits[i]);
    }

  /* Same for ThreadSanitizer: when -fcontracts-p3100 resolves the thread check
     to assume, TSan must not instrument this function (byte identical to a
     build without -fsanitize=thread), realized per function via no_sanitize so
     it streams through LTO -- mirror of the ASan assume line above.  */
  if (flag_contracts_p3100
      && (flag_sanitize & SANITIZE_THREAD)
      && resolved_sanitizer_semantic (SANITIZE_THREAD) == CES_ASSUME)
    add_no_sanitize_value (fndecl, SANITIZE_THREAD);
}
