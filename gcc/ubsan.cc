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
#include "backend.h"
#include "rtl.h"
#include "c-family/c-common.h"
#include "gimple.h"
#include "cfghooks.h"
#include "tree-pass.h"
#include "memmodel.h"
#include "tm_p.h"
#include "ssa.h"
#include "cgraph.h"
#include "tree-pretty-print.h"
#include "stor-layout.h"
#include "cfganal.h"
#include "gimple-iterator.h"
#include "output.h"
#include "cfgloop.h"
#include "ubsan.h"
#include "expr.h"
#include "stringpool.h"
#include "attribs.h"
#include "asan.h"
#include "gimplify-me.h"
#include "dfp.h"
#include "builtins.h"
#include "tree-object-size.h"
#include "tree-cfg.h"
#include "gimple-fold.h"
#include "varasm.h"
#include "realmpfr.h"
#include "target.h"
#include "langhooks.h"

/* Map from a tree to a VAR_DECL tree.  */

struct GTY((for_user)) tree_type_map {
  struct tree_map_base type;
  tree decl;
};

struct tree_type_map_cache_hasher : ggc_cache_ptr_hash<tree_type_map>
{
  static inline hashval_t
  hash (tree_type_map *t)
  {
    return TYPE_UID (t->type.from);
  }

  static inline bool
  equal (tree_type_map *a, tree_type_map *b)
  {
    return a->type.from == b->type.from;
  }

  static int
  keep_cache_entry (tree_type_map *&m)
  {
    return ggc_marked_p (m->type.from);
  }
};

static GTY ((cache))
     hash_table<tree_type_map_cache_hasher> *decl_tree_for_type;

/* Lookup a VAR_DECL for TYPE, and return it if we find one.  */

static tree
decl_for_type_lookup (tree type)
{
  /* If the hash table is not initialized yet, create it now.  */
  if (decl_tree_for_type == NULL)
    {
      decl_tree_for_type
	= hash_table<tree_type_map_cache_hasher>::create_ggc (10);
      /* That also means we don't have to bother with the lookup.  */
      return NULL_TREE;
    }

  struct tree_type_map *h, in;
  in.type.from = type;

  h = decl_tree_for_type->find_with_hash (&in, TYPE_UID (type));
  return h ? h->decl : NULL_TREE;
}

/* Insert a mapping TYPE->DECL in the VAR_DECL for type hashtable.  */

static void
decl_for_type_insert (tree type, tree decl)
{
  struct tree_type_map *h;

  h = ggc_alloc<tree_type_map> ();
  h->type.from = type;
  h->decl = decl;
  *decl_tree_for_type->find_slot_with_hash (h, TYPE_UID (type), INSERT) = h;
}

/* Helper routine, which encodes a value in the pointer_sized_int_node.
   Arguments with precision <= POINTER_SIZE are passed directly,
   the rest is passed by reference.  T is a value we are to encode.
   PHASE determines when this function is called.  */

tree
ubsan_encode_value (tree t, enum ubsan_encode_value_phase phase)
{
  tree type = TREE_TYPE (t);
  if (BITINT_TYPE_P (type))
    {
      if (TYPE_PRECISION (type) <= POINTER_SIZE)
	{
	  type = pointer_sized_int_node;
	  t = fold_build1 (NOP_EXPR, type, t);
	}
      else
	{
	  if (TYPE_PRECISION (type) > MAX_FIXED_MODE_SIZE)
	    return build_zero_cst (pointer_sized_int_node);
	  type = build_nonstandard_integer_type (MAX_FIXED_MODE_SIZE,
	  					 TYPE_UNSIGNED (type));
	  t = fold_build1 (NOP_EXPR, type, t);
	}
    }
  scalar_mode mode = SCALAR_TYPE_MODE (type);
  const unsigned int bitsize = GET_MODE_BITSIZE (mode);
  if (bitsize <= POINTER_SIZE)
    switch (TREE_CODE (type))
      {
      case BOOLEAN_TYPE:
      case ENUMERAL_TYPE:
      case INTEGER_TYPE:
	return fold_build1 (NOP_EXPR, pointer_sized_int_node, t);
      case REAL_TYPE:
	{
	  tree itype = build_nonstandard_integer_type (bitsize, true);
	  t = fold_build1 (VIEW_CONVERT_EXPR, itype, t);
	  return fold_convert (pointer_sized_int_node, t);
	}
      default:
	gcc_unreachable ();
      }
  else
    {
      if (!DECL_P (t) || !TREE_ADDRESSABLE (t))
	{
	  /* The reason for this is that we don't want to pessimize
	     code by making vars unnecessarily addressable.  */
	  tree var;
	  if (phase != UBSAN_ENCODE_VALUE_GENERIC)
	    {
	      var = create_tmp_var (type);
	      mark_addressable (var);
	    }
	  else
	    {
	      var = create_tmp_var_raw (type);
	      TREE_ADDRESSABLE (var) = 1;
	      DECL_CONTEXT (var) = current_function_decl;
	    }
	  if (phase == UBSAN_ENCODE_VALUE_RTL)
	    {
	      rtx mem = assign_stack_temp_for_type (mode, GET_MODE_SIZE (mode),
						    type);
	      SET_DECL_RTL (var, mem);
	      expand_assignment (var, t, false);
	      return build_fold_addr_expr (var);
	    }
	  if (phase != UBSAN_ENCODE_VALUE_GENERIC)
	    {
	      tree tem = build2 (MODIFY_EXPR, void_type_node, var, t);
	      t = build_fold_addr_expr (var);
	      return build2 (COMPOUND_EXPR, TREE_TYPE (t), tem, t);
	    }
	  else
	    {
	      var = build4 (TARGET_EXPR, type, var, t, NULL_TREE, NULL_TREE);
	      return build_fold_addr_expr (var);
	    }
	}
      else
	return build_fold_addr_expr (t);
    }
}

/* Cached ubsan_get_type_descriptor_type () return value.  */
static GTY(()) tree ubsan_type_descriptor_type;

/* Build
   struct __ubsan_type_descriptor
   {
     unsigned short __typekind;
     unsigned short __typeinfo;
     char __typename[];
   }
   type.  */

static tree
ubsan_get_type_descriptor_type (void)
{
  static const char *field_names[3]
    = { "__typekind", "__typeinfo", "__typename" };
  tree fields[3], ret;

  if (ubsan_type_descriptor_type)
    return ubsan_type_descriptor_type;

  tree itype = build_range_type (sizetype, size_zero_node, NULL_TREE);
  tree flex_arr_type = build_array_type (char_type_node, itype);

  ret = make_node (RECORD_TYPE);
  for (int i = 0; i < 3; i++)
    {
      fields[i] = build_decl (UNKNOWN_LOCATION, FIELD_DECL,
			      get_identifier (field_names[i]),
			      (i == 2) ? flex_arr_type
			      : short_unsigned_type_node);
      DECL_CONTEXT (fields[i]) = ret;
      if (i)
	DECL_CHAIN (fields[i - 1]) = fields[i];
    }
  tree type_decl = build_decl (input_location, TYPE_DECL,
			       get_identifier ("__ubsan_type_descriptor"),
			       ret);
  DECL_IGNORED_P (type_decl) = 1;
  DECL_ARTIFICIAL (type_decl) = 1;
  TYPE_FIELDS (ret) = fields[0];
  TYPE_NAME (ret) = type_decl;
  TYPE_STUB_DECL (ret) = type_decl;
  TYPE_ARTIFICIAL (ret) = 1;
  layout_type (ret);
  ubsan_type_descriptor_type = ret;
  return ret;
}

/* Cached ubsan_get_source_location_type () return value.  */
static GTY(()) tree ubsan_source_location_type;

/* Build
   struct __ubsan_source_location
   {
     const char *__filename;
     unsigned int __line;
     unsigned int __column;
   }
   type.  */

tree
ubsan_get_source_location_type (void)
{
  static const char *field_names[3]
    = { "__filename", "__line", "__column" };
  tree fields[3], ret;
  if (ubsan_source_location_type)
    return ubsan_source_location_type;

  tree const_char_type = build_qualified_type (char_type_node,
					       TYPE_QUAL_CONST);

  ret = make_node (RECORD_TYPE);
  for (int i = 0; i < 3; i++)
    {
      fields[i] = build_decl (UNKNOWN_LOCATION, FIELD_DECL,
			      get_identifier (field_names[i]),
			      (i == 0) ? build_pointer_type (const_char_type)
			      : unsigned_type_node);
      DECL_CONTEXT (fields[i]) = ret;
      if (i)
	DECL_CHAIN (fields[i - 1]) = fields[i];
    }
  tree type_decl = build_decl (input_location, TYPE_DECL,
			       get_identifier ("__ubsan_source_location"),
			       ret);
  DECL_IGNORED_P (type_decl) = 1;
  DECL_ARTIFICIAL (type_decl) = 1;
  TYPE_FIELDS (ret) = fields[0];
  TYPE_NAME (ret) = type_decl;
  TYPE_STUB_DECL (ret) = type_decl;
  TYPE_ARTIFICIAL (ret) = 1;
  layout_type (ret);
  ubsan_source_location_type = ret;
  return ret;
}

/* Helper routine that returns a CONSTRUCTOR of __ubsan_source_location
   type with its fields filled from a location_t LOC.  */

static tree
ubsan_source_location (location_t loc)
{
  expanded_location xloc;
  tree type = ubsan_get_source_location_type ();

  xloc = expand_location (loc);
  tree str;
  if (xloc.file == NULL)
    {
      str = build_int_cst (ptr_type_node, 0);
      xloc.line = 0;
      xloc.column = 0;
    }
  else
    {
      /* Fill in the values from LOC.  */
      size_t len = strlen (xloc.file) + 1;
      str = build_string (len, xloc.file);
      TREE_TYPE (str) = build_array_type_nelts (char_type_node, len);
      TREE_READONLY (str) = 1;
      TREE_STATIC (str) = 1;
      str = build_fold_addr_expr (str);
    }
  tree ctor = build_constructor_va (type, 3, NULL_TREE, str, NULL_TREE,
				    build_int_cst (unsigned_type_node,
						   xloc.line), NULL_TREE,
				    build_int_cst (unsigned_type_node,
						   xloc.column));
  TREE_CONSTANT (ctor) = 1;
  TREE_STATIC (ctor) = 1;

  return ctor;
}

/* This routine returns a magic number for TYPE.  */

static unsigned short
get_ubsan_type_info_for_type (tree type)
{
  if (SCALAR_FLOAT_TYPE_P (type))
    return tree_to_uhwi (TYPE_SIZE (type));
  else if (INTEGRAL_TYPE_P (type))
    {
      int prec = exact_log2 (tree_to_uhwi (TYPE_SIZE (type)));
      gcc_assert (prec != -1);
      return (prec << 1) | !TYPE_UNSIGNED (type);
    }
  else
    return 0;
}

/* Helper routine that returns ADDR_EXPR of a VAR_DECL of a type
   descriptor.  It first looks into the hash table; if not found,
   create the VAR_DECL, put it into the hash table and return the
   ADDR_EXPR of it.  TYPE describes a particular type.  PSTYLE is
   an enum controlling how we want to print the type.  */

tree
ubsan_type_descriptor (tree type, enum ubsan_print_style pstyle)
{
  /* See through any typedefs.  */
  type = TYPE_MAIN_VARIANT (type);
  tree type3 = type;
  if (pstyle == UBSAN_PRINT_FORCE_INT)
    {
      /* Temporary hack for -fsanitize=shift with _BitInt(129) and more.
	 libubsan crashes if it is not TK_Integer type.  */
      if (BITINT_TYPE_P (type) && TYPE_PRECISION (type) > MAX_FIXED_MODE_SIZE)
	type3 = build_qualified_type (type, TYPE_QUAL_CONST);
      if (type3 == type)
	pstyle = UBSAN_PRINT_NORMAL;
    }

  tree decl = decl_for_type_lookup (type3);
  /* It is possible that some of the earlier created DECLs were found
     unused, in that case they weren't emitted and varpool_node::get
     returns NULL node on them.  But now we really need them.  Thus,
     renew them here.  */
  if (decl != NULL_TREE && varpool_node::get (decl))
    {
      return build_fold_addr_expr (decl);
    }

  tree dtype = ubsan_get_type_descriptor_type ();
  tree type2 = type;
  const char *tname = NULL;
  pretty_printer pretty_name;
  unsigned char deref_depth = 0;
  unsigned short tkind, tinfo;
  char tname_bitint[sizeof ("unsigned _BitInt(2147483647)")];

  /* Get the name of the type, or the name of the pointer type.  */
  if (pstyle == UBSAN_PRINT_POINTER)
    {
      gcc_assert (POINTER_TYPE_P (type));
      type2 = TREE_TYPE (type);

      /* Remove any '*' operators from TYPE.  */
      while (POINTER_TYPE_P (type2))
        deref_depth++, type2 = TREE_TYPE (type2);

      if (TREE_CODE (type2) == METHOD_TYPE)
        type2 = TYPE_METHOD_BASETYPE (type2);
    }

  /* If an array, get its type.  */
  type2 = strip_array_types (type2);

  if (pstyle == UBSAN_PRINT_ARRAY)
    {
      while (POINTER_TYPE_P (type2))
        deref_depth++, type2 = TREE_TYPE (type2);
    }

  if (TYPE_NAME (type2) != NULL)
    {
      if (TREE_CODE (TYPE_NAME (type2)) == IDENTIFIER_NODE)
	tname = IDENTIFIER_POINTER (TYPE_NAME (type2));
      else if (DECL_NAME (TYPE_NAME (type2)) != NULL)
	tname = IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (type2)));
    }

  if (tname == NULL)
    {
      if (BITINT_TYPE_P (type2))
	{
	  snprintf (tname_bitint, sizeof (tname_bitint),
	  	    "%s_BitInt(%d)", TYPE_UNSIGNED (type2) ? "unsigned " : "",
		    TYPE_PRECISION (type2));
	  tname = tname_bitint;
	}
      else
	/* We weren't able to determine the type name.  */
	tname = "<unknown>";
    }

  pp_quote (&pretty_name);

  tree eltype = type;
  if (pstyle == UBSAN_PRINT_POINTER)
    {
      pp_printf (&pretty_name, "%s%s%s%s%s%s%s",
		 TYPE_VOLATILE (type2) ? "volatile " : "",
		 TYPE_READONLY (type2) ? "const " : "",
		 TYPE_RESTRICT (type2) ? "restrict " : "",
		 TYPE_ATOMIC (type2) ? "_Atomic " : "",
		 TREE_CODE (type2) == RECORD_TYPE
		 ? "struct "
		 : TREE_CODE (type2) == UNION_TYPE
		   ? "union " : "", tname,
		 deref_depth == 0 ? "" : " ");
      while (deref_depth-- > 0)
	pp_star (&pretty_name);
    }
  else if (pstyle == UBSAN_PRINT_ARRAY)
    {
      /* Pretty print the array dimensions.  */
      gcc_assert (TREE_CODE (type) == ARRAY_TYPE);
      tree t = type;
      pp_string (&pretty_name, tname);
      pp_space (&pretty_name);
      while (deref_depth-- > 0)
	pp_star (&pretty_name);
      while (TREE_CODE (t) == ARRAY_TYPE)
	{
	  pp_left_bracket (&pretty_name);
	  tree dom = TYPE_DOMAIN (t);
	  if (dom != NULL_TREE
	      && TYPE_MAX_VALUE (dom) != NULL_TREE
	      && TREE_CODE (TYPE_MAX_VALUE (dom)) == INTEGER_CST)
	    {
	      unsigned HOST_WIDE_INT m;
	      if (tree_fits_uhwi_p (TYPE_MAX_VALUE (dom))
		  && (m = tree_to_uhwi (TYPE_MAX_VALUE (dom))) + 1 != 0)
		pp_unsigned_wide_integer (&pretty_name, m + 1);
	      else
		pp_wide_int (&pretty_name,
			     wi::add (wi::to_widest (TYPE_MAX_VALUE (dom)), 1),
			     TYPE_SIGN (TREE_TYPE (dom)));
	    }
	  else
	    /* ??? We can't determine the variable name; print VLA unspec.  */
	    pp_star (&pretty_name);
	  pp_right_bracket (&pretty_name);
	  t = TREE_TYPE (t);
	}

      /* Save the tree with stripped types.  */
      eltype = t;
    }
  else
    pp_string (&pretty_name, tname);

  pp_quote (&pretty_name);

  switch (TREE_CODE (eltype))
    {
    case BOOLEAN_TYPE:
    case INTEGER_TYPE:
      tkind = 0x0000;
      break;
    case ENUMERAL_TYPE:
      if (!BITINT_TYPE_P (eltype))
	{
	  tkind = 0x0000;
	  break;
	}
      /* FALLTHRU */
    case BITINT_TYPE:
      if (TYPE_PRECISION (eltype) <= MAX_FIXED_MODE_SIZE)
	tkind = 0x0000;
      else
	tkind = 0xffff;
      break;
    case REAL_TYPE:
      /* FIXME: libubsan right now only supports float, double and
	 long double type formats.  */
      if (TYPE_MODE (eltype) == TYPE_MODE (float_type_node)
	  || TYPE_MODE (eltype) == TYPE_MODE (double_type_node)
	  || TYPE_MODE (eltype) == TYPE_MODE (long_double_type_node))
	tkind = 0x0001;
      else
	tkind = 0xffff;
      break;
    default:
      tkind = 0xffff;
      break;
    }
  tinfo = tkind == 0xffff ? 0 : get_ubsan_type_info_for_type (eltype);

  if (pstyle == UBSAN_PRINT_FORCE_INT)
    {
      tkind = 0x0000;
      tree t = build_nonstandard_integer_type (MAX_FIXED_MODE_SIZE,
					       TYPE_UNSIGNED (eltype));
      tinfo = get_ubsan_type_info_for_type (t);
    }

  /* Create a new VAR_DECL of type descriptor.  */
  const char *tmp = pp_formatted_text (&pretty_name);
  size_t len = strlen (tmp) + 1;
  tree str = build_string (len, tmp);
  TREE_TYPE (str) = build_array_type_nelts (char_type_node, len);
  TREE_READONLY (str) = 1;
  TREE_STATIC (str) = 1;

  decl = build_decl (UNKNOWN_LOCATION, VAR_DECL,
		     generate_internal_label ("Lubsan_type"), dtype);
  TREE_STATIC (decl) = 1;
  TREE_PUBLIC (decl) = 0;
  DECL_ARTIFICIAL (decl) = 1;
  DECL_IGNORED_P (decl) = 1;
  DECL_EXTERNAL (decl) = 0;
  DECL_SIZE (decl)
    = size_binop (PLUS_EXPR, DECL_SIZE (decl), TYPE_SIZE (TREE_TYPE (str)));
  DECL_SIZE_UNIT (decl)
    = size_binop (PLUS_EXPR, DECL_SIZE_UNIT (decl),
		  TYPE_SIZE_UNIT (TREE_TYPE (str)));

  tree ctor = build_constructor_va (dtype, 3, NULL_TREE,
				    build_int_cst (short_unsigned_type_node,
						   tkind), NULL_TREE,
				    build_int_cst (short_unsigned_type_node,
						   tinfo), NULL_TREE, str);
  TREE_CONSTANT (ctor) = 1;
  TREE_STATIC (ctor) = 1;
  DECL_INITIAL (decl) = ctor;
  varpool_node::finalize_decl (decl);

  /* Save the VAR_DECL into the hash table.  */
  decl_for_type_insert (type3, decl);

  return build_fold_addr_expr (decl);
}

/* Create a structure for the ubsan library.  NAME is a name of the new
   structure.  LOCCNT is number of locations, PLOC points to array of
   locations.  The arguments in ... are of __ubsan_type_descriptor type
   and there are at most two of them, followed by NULL_TREE, followed
   by optional extra arguments and another NULL_TREE.  */

tree
ubsan_create_data (const char *name, int loccnt, const location_t *ploc, ...)
{
  va_list args;
  tree ret, t;
  tree fields[6];
  vec<tree, va_gc> *saved_args = NULL;
  size_t i = 0;
  int j;

  /* It is possible that PCH zapped table with definitions of sanitizer
     builtins.  Reinitialize them if needed.  */
  initialize_sanitizer_builtins ();

  /* Firstly, create a pointer to type descriptor type.  */
  tree td_type = ubsan_get_type_descriptor_type ();
  td_type = build_pointer_type (td_type);

  /* Create the structure type.  */
  ret = make_node (RECORD_TYPE);
  for (j = 0; j < loccnt; j++)
    {
      gcc_checking_assert (i < 2);
      fields[i] = build_decl (UNKNOWN_LOCATION, FIELD_DECL, NULL_TREE,
			      ubsan_get_source_location_type ());
      DECL_CONTEXT (fields[i]) = ret;
      if (i)
	DECL_CHAIN (fields[i - 1]) = fields[i];
      i++;
    }

  va_start (args, ploc);
  for (t = va_arg (args, tree); t != NULL_TREE;
       i++, t = va_arg (args, tree))
    {
      gcc_checking_assert (i < 4);
      /* Save the tree arguments for later use.  */
      vec_safe_push (saved_args, t);
      fields[i] = build_decl (UNKNOWN_LOCATION, FIELD_DECL, NULL_TREE,
			      td_type);
      DECL_CONTEXT (fields[i]) = ret;
      if (i)
	DECL_CHAIN (fields[i - 1]) = fields[i];
    }

  for (t = va_arg (args, tree); t != NULL_TREE;
       i++, t = va_arg (args, tree))
    {
      gcc_checking_assert (i < 6);
      /* Save the tree arguments for later use.  */
      vec_safe_push (saved_args, t);
      fields[i] = build_decl (UNKNOWN_LOCATION, FIELD_DECL, NULL_TREE,
			      TREE_TYPE (t));
      DECL_CONTEXT (fields[i]) = ret;
      if (i)
	DECL_CHAIN (fields[i - 1]) = fields[i];
    }
  va_end (args);

  tree type_decl = build_decl (input_location, TYPE_DECL,
			       get_identifier (name), ret);
  DECL_IGNORED_P (type_decl) = 1;
  DECL_ARTIFICIAL (type_decl) = 1;
  TYPE_FIELDS (ret) = fields[0];
  TYPE_NAME (ret) = type_decl;
  TYPE_STUB_DECL (ret) = type_decl;
  TYPE_ARTIFICIAL (ret) = 1;
  layout_type (ret);

  /* Now, fill in the type.  */
  tree var = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			 generate_internal_label ("Lubsan_data"), ret);
  TREE_STATIC (var) = 1;
  TREE_PUBLIC (var) = 0;
  DECL_ARTIFICIAL (var) = 1;
  DECL_IGNORED_P (var) = 1;
  DECL_EXTERNAL (var) = 0;

  vec<constructor_elt, va_gc> *v;
  vec_alloc (v, i);
  tree ctor = build_constructor (ret, v);

  /* If desirable, set the __ubsan_source_location element.  */
  for (j = 0; j < loccnt; j++)
    {
      location_t loc = LOCATION_LOCUS (ploc[j]);
      CONSTRUCTOR_APPEND_ELT (v, NULL_TREE, ubsan_source_location (loc));
    }

  size_t nelts = vec_safe_length (saved_args);
  for (i = 0; i < nelts; i++)
    {
      t = (*saved_args)[i];
      CONSTRUCTOR_APPEND_ELT (v, NULL_TREE, t);
    }

  TREE_CONSTANT (ctor) = 1;
  TREE_STATIC (ctor) = 1;
  DECL_INITIAL (var) = ctor;
  varpool_node::finalize_decl (var);

  return var;
}

/* Shared between *build_builtin_unreachable.  */

tree
sanitize_unreachable_fn (tree *data, location_t loc)
{
  tree fn = NULL_TREE;
  bool san = sanitize_flags_p (SANITIZE_UNREACHABLE);
  if (san
      ? (flag_sanitize_trap & SANITIZE_UNREACHABLE)
      : flag_unreachable_traps)
    {
      fn = builtin_decl_explicit (BUILT_IN_UNREACHABLE_TRAP);
      *data = NULL_TREE;
    }
  else if (san)
    {
      /* Call ubsan_create_data first as it initializes SANITIZER built-ins.  */
      *data = ubsan_create_data ("__ubsan_unreachable_data", 1, &loc,
				 NULL_TREE, NULL_TREE);
      fn = builtin_decl_explicit (BUILT_IN_UBSAN_HANDLE_BUILTIN_UNREACHABLE);
      *data = build_fold_addr_expr_loc (loc, *data);
    }
  else
    {
      fn = builtin_decl_explicit (BUILT_IN_UNREACHABLE);
      *data = NULL_TREE;
    }
  return fn;
}

/* Rewrite a gcall to __builtin_unreachable for -fsanitize=unreachable.  Called
   by the sanopt pass.  */

bool
ubsan_instrument_unreachable (gimple_stmt_iterator *gsi)
{
  location_t loc = gimple_location (gsi_stmt (*gsi));
  gimple *g = gimple_build_builtin_unreachable (loc);
  gsi_replace (gsi, g, false);
  return false;
}

/* Return true if T is a call to a libubsan routine.  */

bool
is_ubsan_builtin_p (tree t)
{
  return TREE_CODE (t) == FUNCTION_DECL
	 && fndecl_built_in_p (t, BUILT_IN_NORMAL)
	 && strncmp (IDENTIFIER_POINTER (DECL_NAME (t)),
		     "__builtin___ubsan_", 18) == 0;
}

/* Create a callgraph edge for statement STMT.  */

static void
ubsan_create_edge (gimple *stmt)
{
  gcall *call_stmt = dyn_cast <gcall *> (stmt);
  basic_block bb = gimple_bb (stmt);
  cgraph_node *node = cgraph_node::get (current_function_decl);
  tree decl = gimple_call_fndecl (call_stmt);
  if (decl)
    node->create_edge (cgraph_node::get_create (decl), call_stmt, bb->count);
}

/* Expand the UBSAN_BOUNDS special builtin function.  */

bool
ubsan_expand_bounds_ifn (gimple_stmt_iterator *gsi)
{
  gimple *stmt = gsi_stmt (*gsi);
  location_t loc = gimple_location (stmt);
  gcc_assert (gimple_call_num_args (stmt) == 3);

  /* Pick up the arguments of the UBSAN_BOUNDS call.  */
  tree type = TREE_TYPE (TREE_TYPE (gimple_call_arg (stmt, 0)));
  tree index = gimple_call_arg (stmt, 1);
  tree orig_index = index;
  tree bound = gimple_call_arg (stmt, 2);

  gimple_stmt_iterator gsi_orig = *gsi;

  /* Create condition "if (index >= bound)".  */
  basic_block then_bb, fallthru_bb;
  gimple_stmt_iterator cond_insert_point
    = create_cond_insert_point (gsi, false, false, true,
				&then_bb, &fallthru_bb);
  index = fold_convert (TREE_TYPE (bound), index);
  index = force_gimple_operand_gsi (&cond_insert_point, index,
				    true, NULL_TREE,
				    false, GSI_NEW_STMT);
  gimple *g = gimple_build_cond (GE_EXPR, index, bound, NULL_TREE, NULL_TREE);
  gimple_set_location (g, loc);
  gsi_insert_after (&cond_insert_point, g, GSI_NEW_STMT);

  /* Generate __ubsan_handle_out_of_bounds call.  */
  *gsi = gsi_after_labels (then_bb);
  if (flag_sanitize_trap & SANITIZE_BOUNDS)
    g = gimple_build_call (builtin_decl_explicit (BUILT_IN_TRAP), 0);
  else
    {
      tree data
	= ubsan_create_data ("__ubsan_out_of_bounds_data", 1, &loc,
			     ubsan_type_descriptor (type, UBSAN_PRINT_ARRAY),
			     ubsan_type_descriptor (TREE_TYPE (orig_index)),
			     NULL_TREE, NULL_TREE);
      data = build_fold_addr_expr_loc (loc, data);
      enum built_in_function bcode
	= (flag_sanitize_recover & SANITIZE_BOUNDS)
	  ? BUILT_IN_UBSAN_HANDLE_OUT_OF_BOUNDS
	  : BUILT_IN_UBSAN_HANDLE_OUT_OF_BOUNDS_ABORT;
      tree fn = builtin_decl_explicit (bcode);
      tree val = ubsan_encode_value (orig_index, UBSAN_ENCODE_VALUE_GIMPLE);
      val = force_gimple_operand_gsi (gsi, val, true, NULL_TREE, true,
				      GSI_SAME_STMT);
      g = gimple_build_call (fn, 2, data, val);
    }
  gimple_set_location (g, loc);
  gsi_insert_before (gsi, g, GSI_SAME_STMT);

  /* Get rid of the UBSAN_BOUNDS call from the IR.  */
  unlink_stmt_vdef (stmt);
  gsi_remove (&gsi_orig, true);

  /* Point GSI to next logical statement.  */
  *gsi = gsi_start_bb (fallthru_bb);
  return true;
}

/* P3100: return the language-neutral reaction (enum implicit_ub_reaction) for
   an implicit null-pointer-dereference contract assertion at LOC in the current
   function, or IMPLICIT_UB_NONE when P3100 is off or the site resolves to
   assume/ignore.  Resolution is per site (LOC feeds per-file/line config
   matching) and per enclosing namespace (from cfun->decl); see
   cp_resolve_implicit_ub_semantic.  */

static int
implicit_null_deref_reaction (location_t loc)
{
  if (!flag_contracts_p3100)
    return IMPLICIT_UB_NONE;
  return lang_hooks.resolve_implicit_ub_semantic (cfun->decl, loc,
						  "ub:expr.unary.dereference.nullptr");
}

/* P3100: the reaction for an implicit misaligned-access contract assertion
   (ub:basic.align.object.alignment) at LOC in the current function, or
   IMPLICIT_UB_NONE when P3100 is off or the site resolves to assume.  Like
   null-deref, a misaligned access is an lvalue with no defined substitute, so
   "ignore" resolves to IMPLICIT_UB_NONE (the raw access) rather than
   IMPLICIT_UB_DEFINED.  */

static int
implicit_align_reaction (location_t loc)
{
  if (!flag_contracts_p3100)
    return IMPLICIT_UB_NONE;
  return lang_hooks.resolve_implicit_ub_semantic (cfun->decl, loc,
						  "ub:basic.align.object.alignment");
}

/* P3100: the reaction for an implicit signed-integer-overflow contract
   assertion (ub:expr.expr.eval.signed.integer) at LOC in the current function,
   or IMPLICIT_UB_NONE when P3100 is off or the site resolves to assume.
   Unlike null-deref, "ignore" here resolves to IMPLICIT_UB_DEFINED (the
   operation must still be instrumented so its result is the defined wrapped
   value and the optimizer stops assuming
   no overflow).  */

static int
implicit_overflow_reaction (location_t loc)
{
  if (!flag_contracts_p3100)
    return IMPLICIT_UB_NONE;
  return lang_hooks.resolve_implicit_ub_semantic (cfun->decl, loc,
						  "ub:expr.expr.eval.signed.integer");
}

/* P3100: the reaction for an implicit invalid-value-load contract assertion
   (ub:conv.lval.valid.representation) at LOC in the current function, or
   IMPLICIT_UB_NONE when P3100 is off or the site resolves to assume.  Like
   signed overflow, "ignore" resolves to IMPLICIT_UB_DEFINED here: the load is
   instrumented so an out-of-range bool/enum value is replaced by a defined
   valid value (0).  */

static int
implicit_invalid_value_reaction (location_t loc)
{
  if (!flag_contracts_p3100)
    return IMPLICIT_UB_NONE;
  return lang_hooks.resolve_implicit_ub_semantic
	   (cfun->decl, loc, "ub:conv.lval.valid.representation.bool.enum");
}

/* Expand UBSAN_NULL internal call.  The type is kept on the ckind
   argument which is a constant, because the middle-end treats pointer
   conversions as useless and therefore the type of the first argument
   could be changed to any other pointer type.  */

bool
ubsan_expand_null_ifn (gimple_stmt_iterator *gsip)
{
  gimple_stmt_iterator gsi = *gsip;
  gimple *stmt = gsi_stmt (gsi);
  location_t loc = gimple_location (stmt);
  gcc_assert (gimple_call_num_args (stmt) == UBSAN_NULL_NUM_OPS);
  tree ptr = gimple_call_arg (stmt, UBSAN_NULL_PTR);
  tree ckind = gimple_call_arg (stmt, UBSAN_NULL_CKIND);
  tree align = gimple_call_arg (stmt, UBSAN_NULL_ALIGN);
  tree check_align = NULL_TREE;
  bool check_null;

  /* P3100: does an implicit ub:expr.unary.dereference.nullptr contract
     assertion apply at this site?  The reaction was resolved once at pass_ubsan
     (pre-inline), where cfun->decl was the true enclosing function whose
     namespace the contract config matched, and carried here as operand 3.  We
     must NOT re-resolve it against the current cfun->decl: after inlining this
     statement may live in a caller in a different namespace, where the config
     would no longer match and the check would be wrongly dropped.  When it
     does apply, its reaction takes precedence over the sanitizer's on the null
     edge.  IMPLICIT_UB_NONE (0) means no contract -- the pure-sanitizer path,
     byte-for-byte as before.  */
  int p3100_reaction
    = tree_to_shwi (gimple_call_arg (stmt, UBSAN_NULL_REACTION));
  bool have_contract = (p3100_reaction != IMPLICIT_UB_NONE);
  /* P3100: the alignment sub-condition carries its OWN reaction (operand 6) and
     handler entry/data (operands 7/8), resolved independently of the null one at
     pass_ubsan.  IMPLICIT_UB_NONE means "no alignment contract" (sanitizer or
     nothing).  */
  int align_reaction
    = tree_to_shwi (gimple_call_arg (stmt, UBSAN_NULL_ALIGN_REACTION));
  bool have_align_contract = (align_reaction != IMPLICIT_UB_NONE);
  /* Read the alignment handler entry/data now, while STMT is still the IFN call:
     the null-check lowering below reassigns STMT to the GIMPLE_COND it replaces
     the call with, after which gimple_call_arg would no longer work.  */
  tree align_entry = gimple_call_arg (stmt, UBSAN_NULL_ALIGN_ENTRY);
  tree align_data = gimple_call_arg (stmt, UBSAN_NULL_ALIGN_DATA);

  basic_block cur_bb = gsi_bb (gsi);

  gimple *g;
  if (!integer_zerop (align))
    {
      unsigned int ptralign = get_pointer_alignment (ptr) / BITS_PER_UNIT;
      if (compare_tree_int (align, ptralign) == 1)
	{
	  check_align = make_ssa_name (pointer_sized_int_node);
	  g = gimple_build_assign (check_align, NOP_EXPR, ptr);
	  gimple_set_location (g, loc);
	  gsi_insert_before (&gsi, g, GSI_SAME_STMT);
	}
    }
  check_null = sanitize_flags_p (SANITIZE_NULL) || have_contract;
  if (check_null && POINTER_TYPE_P (TREE_TYPE (ptr)))
    {
      addr_space_t as = TYPE_ADDR_SPACE (TREE_TYPE (TREE_TYPE (ptr)));
      if (!ADDR_SPACE_GENERIC_P (as)
	  && targetm.addr_space.zero_address_valid (as))
	check_null = false;
    }

  if (check_align == NULL_TREE && !check_null)
    {
      gsi_remove (gsip, true);
      /* Unlink the UBSAN_NULLs vops before replacing it.  */
      unlink_stmt_vdef (stmt);
      return true;
    }

  /* Split the original block holding the pointer dereference.  */
  edge e = split_block (cur_bb, stmt);

  /* Get a hold on the 'condition block', the 'then block' and the
     'else block'.  */
  basic_block cond_bb = e->src;
  basic_block fallthru_bb = e->dest;
  basic_block then_bb = create_empty_bb (cond_bb);
  if (current_loops)
    {
      add_bb_to_loop (then_bb, cond_bb->loop_father);
      loops_state_set (LOOPS_NEED_FIXUP);
    }

  /* Make an edge coming from the 'cond block' into the 'then block';
     this edge is unlikely taken, so set up the probability accordingly.  */
  e = make_edge (cond_bb, then_bb, EDGE_TRUE_VALUE);
  e->probability = profile_probability::very_unlikely ();
  then_bb->count = e->count ();

  /* Connect 'then block' with the 'else block'.  This is needed
     as the ubsan routines we call in the 'then block' are not noreturn.
     The 'then block' only has one outcoming edge.  */
  make_single_succ_edge (then_bb, fallthru_bb, EDGE_FALLTHRU);

  /* Set up the fallthrough basic block.  */
  e = find_edge (cond_bb, fallthru_bb);
  e->flags = EDGE_FALSE_VALUE;
  e->probability = profile_probability::very_likely ();

  /* Update dominance info for the newly created then_bb; note that
     fallthru_bb's dominance info has already been updated by
     split_block.  */
  if (dom_info_available_p (CDI_DOMINATORS))
    set_immediate_dominator (CDI_DOMINATORS, then_bb, cond_bb);

  /* P3100: a contract assertion on this dereference uses the contract reaction,
     taking precedence over the sanitizer.  This then_bb is the NULL edge's
     reaction (null contract, operands 3/4/5) -- or, in the alignment-only case
     (no null edge), the alignment reaction (operands 6/7/8).  In the both-edges
     case the ALIGNMENT edge gets its own separate then-block further below, so a
     null-deref contract and an alignment contract at the same access each fire
     with their own semantic.  */
  bool null_contract_edge = (have_contract && check_null);
  bool align_contract_edge
    = (have_align_contract && check_align != NULL_TREE && !check_null);
  bool use_contract_reaction = null_contract_edge || align_contract_edge;

  /* Build the reaction gimple for one edge of the check.  When REACTION is a
     P3100 contract reaction it is a nothrow handler call (noexcept_enforce/
     observe) or a trap (quick_enforce); when REACTION is IMPLICIT_UB_NONE it
     is the stock -fsanitize= reaction (trap, or the
     __ubsan_handle_type_mismatch_v1[_abort] handler) selected by
     SANITIZE_MASK.  ENTRY/DATA_ADDR are the handler entry point and static
     data-block address carried on the IFN (built at pass_ubsan while the C++
     langhook was still available); PTR_ARG is the pointer operand the
     sanitizer handler wants.  The null and alignment edges share this builder
     so their reactions stay in lockstep.  */
  auto build_reaction = [&] (int reaction, tree entry, tree data_addr,
			     unsigned sanitize_mask, tree ptr_arg) -> gimple *
    {
      if (reaction != IMPLICIT_UB_NONE)
	{
	  /* The entry/data were built at pass_ubsan (front-end present,
	     pre-inline cfun->decl) and carried on the IFN.  We must NOT
	     rebuild them here via the langhook: this runs post-inline and,
	     under LTO, at LTRANS where no C++ langhook exists (it would return
	     false and degrade to a trap).  A null entry means quick_enforce /
	     no handler -> trap.  */
	  if ((reaction == IMPLICIT_UB_NOEXCEPT_ENFORCE
	       || reaction == IMPLICIT_UB_NOEXCEPT_OBSERVE)
	      && entry != NULL_TREE && !integer_zerop (entry))
	    /* noexcept_enforce / noexcept_observe: call the nothrow contract
	       handler entry point (the decl encodes noreturn for enforce; for
	       observe it returns and the then-block falls through to the real
	       access -- "report then proceed").  No EH region needed.  */
	    return gimple_build_call (entry, 1, data_addr);
	  /* IMPLICIT_UB_TRAP (quick_enforce).  Throwing enforce/observe are
	     excluded from this check's allowed set on the front-end side and
	     clamped away, so they never reach here.  */
	  return gimple_build_call (builtin_decl_implicit (BUILT_IN_TRAP), 0);
	}
      if (flag_sanitize_trap & sanitize_mask)
	return gimple_build_call (builtin_decl_implicit (BUILT_IN_TRAP), 0);
      enum built_in_function bcode
	= (flag_sanitize_recover & sanitize_mask)
	  ? BUILT_IN_UBSAN_HANDLE_TYPE_MISMATCH_V1
	  : BUILT_IN_UBSAN_HANDLE_TYPE_MISMATCH_V1_ABORT;
      tree fn = builtin_decl_implicit (bcode);
      int align_log = tree_log2 (align);
      tree data
	= ubsan_create_data ("__ubsan_null_data", 1, &loc,
			     ubsan_type_descriptor (TREE_TYPE (ckind),
						    UBSAN_PRINT_POINTER),
			     NULL_TREE,
			     build_int_cst (unsigned_char_type_node,
					    MAX (align_log, 0)),
			     fold_convert (unsigned_char_type_node, ckind),
			     NULL_TREE);
      data = build_fold_addr_expr_loc (loc, data);
      return gimple_build_call (fn, 2, data, ptr_arg);
    };

  /* Put the reaction for the first edge (null, or -- in the alignment-only
     case -- alignment) into the newly created then_bb.  STMT is still the IFN
     call here, so its per-edge entry/data operands are readable.  */
  {
    int reaction = use_contract_reaction
		   ? (align_contract_edge ? align_reaction : p3100_reaction)
		   : IMPLICIT_UB_NONE;
    tree entry = use_contract_reaction
		 ? gimple_call_arg (stmt, align_contract_edge
					  ? UBSAN_NULL_ALIGN_ENTRY
					  : UBSAN_NULL_ENTRY)
		 : NULL_TREE;
    tree data_addr = use_contract_reaction
		     ? gimple_call_arg (stmt, align_contract_edge
					      ? UBSAN_NULL_ALIGN_DATA
					      : UBSAN_NULL_DATA)
		     : NULL_TREE;
    unsigned sanitize_mask = (check_align ? SANITIZE_ALIGNMENT + 0 : 0)
			     | (check_null ? SANITIZE_NULL + 0 : 0);
    tree ptr_arg = check_align ? check_align
			       : build_zero_cst (pointer_sized_int_node);
    g = build_reaction (reaction, entry, data_addr, sanitize_mask, ptr_arg);
  }
  gimple_stmt_iterator gsi2 = gsi_start_bb (then_bb);
  gimple_set_location (g, loc);
  gsi_insert_after (&gsi2, g, GSI_NEW_STMT);

  /* Unlink the UBSAN_NULLs vops before replacing it.  */
  unlink_stmt_vdef (stmt);

  if (check_null)
    {
      g = gimple_build_cond (EQ_EXPR, ptr, build_int_cst (TREE_TYPE (ptr), 0),
			     NULL_TREE, NULL_TREE);
      gimple_set_location (g, loc);

      /* Replace the UBSAN_NULL with a GIMPLE_COND stmt.  */
      gsi_replace (&gsi, g, false);
      stmt = g;
    }

  if (check_align)
    {
      basic_block cond2_bb = NULL;
      if (check_null)
	{
	  /* Split the block with the condition again.  */
	  e = split_block (cond_bb, stmt);
	  basic_block cond1_bb = e->src;
	  cond2_bb = e->dest;

	  /* Make an edge coming from the 'cond1 block' into the 'then block';
	     this edge is unlikely taken, so set up the probability
	     accordingly.  */
	  e = make_edge (cond1_bb, then_bb, EDGE_TRUE_VALUE);
	  e->probability = profile_probability::very_unlikely ();

	  /* Set up the fallthrough basic block.  */
	  e = find_edge (cond1_bb, cond2_bb);
	  e->flags = EDGE_FALSE_VALUE;
	  e->probability = profile_probability::very_likely ();

	  /* Update dominance info.  */
	  if (dom_info_available_p (CDI_DOMINATORS))
	    {
	      set_immediate_dominator (CDI_DOMINATORS, fallthru_bb, cond1_bb);
	      set_immediate_dominator (CDI_DOMINATORS, then_bb, cond1_bb);
	    }

	  gsi2 = gsi_start_bb (cond2_bb);
	}

      tree mask = build_int_cst (pointer_sized_int_node,
				 tree_to_uhwi (align) - 1);
      g = gimple_build_assign (make_ssa_name (pointer_sized_int_node),
			       BIT_AND_EXPR, check_align, mask);
      gimple_set_location (g, loc);
      if (check_null)
	gsi_insert_after (&gsi2, g, GSI_NEW_STMT);
      else
	gsi_insert_before (&gsi, g, GSI_SAME_STMT);

      g = gimple_build_cond (NE_EXPR, gimple_assign_lhs (g),
			     build_int_cst (pointer_sized_int_node, 0),
			     NULL_TREE, NULL_TREE);
      gimple_set_location (g, loc);
      if (check_null)
	gsi_insert_after (&gsi2, g, GSI_NEW_STMT);
      else
	/* Replace the UBSAN_NULL with a GIMPLE_COND stmt.  */
	gsi_replace (&gsi, g, false);

      /* P3100: in the both-edges case the alignment condition's TRUE edge was
	 inherited pointing at the shared then_bb (which holds the NULL edge's
	 reaction).  When a contract applies to either edge, give the alignment
	 edge its OWN then-block with its own reaction, so a null-deref contract
	 and an alignment contract at the same access each fire independently.
	 When neither edge is a contract the two share then_bb -- the stock
	 -fsanitize= codegen, unchanged.  */
      if (cond2_bb != NULL && (have_contract || have_align_contract))
	{
	  basic_block then_align_bb = create_empty_bb (cond2_bb);
	  if (current_loops)
	    {
	      add_bb_to_loop (then_align_bb, cond2_bb->loop_father);
	      loops_state_set (LOOPS_NEED_FIXUP);
	    }
	  make_single_succ_edge (then_align_bb, fallthru_bb, EDGE_FALLTHRU);
	  edge te = find_edge (cond2_bb, then_bb);
	  redirect_edge_and_branch (te, then_align_bb);
	  te->probability = profile_probability::very_unlikely ();
	  then_align_bb->count = te->count ();
	  if (dom_info_available_p (CDI_DOMINATORS))
	    set_immediate_dominator (CDI_DOMINATORS, then_align_bb, cond2_bb);

	  /* Build the alignment edge's reaction with the shared builder: its own
	     contract reaction (operands 6/7/8, captured above as align_entry/
	     align_data since STMT is no longer the IFN call) or, when the alignment
	     edge is a pure sanitizer check, the stock type-mismatch handler.  */
	  gimple *ag
	    = build_reaction (have_align_contract ? align_reaction
						  : IMPLICIT_UB_NONE,
			      align_entry, align_data, SANITIZE_ALIGNMENT,
			      check_align);
	  gimple_set_location (ag, loc);
	  gimple_stmt_iterator agsi = gsi_start_bb (then_align_bb);
	  gsi_insert_after (&agsi, ag, GSI_NEW_STMT);
	}
    }
  return false;
}

#define OBJSZ_MAX_OFFSET (1024 * 16)

/* Expand UBSAN_OBJECT_SIZE internal call.  */

bool
ubsan_expand_objsize_ifn (gimple_stmt_iterator *gsi)
{
  gimple *stmt = gsi_stmt (*gsi);
  location_t loc = gimple_location (stmt);
  gcc_assert (gimple_call_num_args (stmt) == 4);

  tree ptr = gimple_call_arg (stmt, 0);
  tree offset = gimple_call_arg (stmt, 1);
  tree size = gimple_call_arg (stmt, 2);
  tree ckind = gimple_call_arg (stmt, 3);
  gimple_stmt_iterator gsi_orig = *gsi;
  gimple *g;

  /* See if we can discard the check.  */
  if (TREE_CODE (size) == INTEGER_CST
      && integer_all_onesp (size))
    /* Yes, __builtin_object_size couldn't determine the
       object size.  */;
  else if (TREE_CODE (offset) == INTEGER_CST
	   && wi::to_widest (offset) >= -OBJSZ_MAX_OFFSET
	   && wi::to_widest (offset) <= -1)
    /* The offset is in range [-16K, -1].  */;
  else
    {
      /* if (offset > objsize) */
      basic_block then_bb, fallthru_bb;
      gimple_stmt_iterator cond_insert_point
	= create_cond_insert_point (gsi, false, false, true,
				    &then_bb, &fallthru_bb);
      g = gimple_build_cond (GT_EXPR, offset, size, NULL_TREE, NULL_TREE);
      gimple_set_location (g, loc);
      gsi_insert_after (&cond_insert_point, g, GSI_NEW_STMT);

      /* If the offset is small enough, we don't need the second
	 run-time check.  */
      if (TREE_CODE (offset) == INTEGER_CST
	  && wi::to_widest (offset) >= 0
	  && wi::to_widest (offset) <= OBJSZ_MAX_OFFSET)
	*gsi = gsi_after_labels (then_bb);
      else
	{
	  /* Don't issue run-time error if (ptr > ptr + offset).  That
	     may happen when computing a POINTER_PLUS_EXPR.  */
	  basic_block then2_bb, fallthru2_bb;

	  gimple_stmt_iterator gsi2 = gsi_after_labels (then_bb);
	  cond_insert_point = create_cond_insert_point (&gsi2, false, false,
							true, &then2_bb,
							&fallthru2_bb);
	  /* Convert the pointer to an integer type.  */
	  tree p = make_ssa_name (pointer_sized_int_node);
	  g = gimple_build_assign (p, NOP_EXPR, ptr);
	  gimple_set_location (g, loc);
	  gsi_insert_before (&cond_insert_point, g, GSI_NEW_STMT);
	  p = gimple_assign_lhs (g);
	  /* Compute ptr + offset.  */
	  g = gimple_build_assign (make_ssa_name (pointer_sized_int_node),
				   PLUS_EXPR, p, offset);
	  gimple_set_location (g, loc);
	  gsi_insert_after (&cond_insert_point, g, GSI_NEW_STMT);
	  /* Now build the conditional and put it into the IR.  */
	  g = gimple_build_cond (LE_EXPR, p, gimple_assign_lhs (g),
				 NULL_TREE, NULL_TREE);
	  gimple_set_location (g, loc);
	  gsi_insert_after (&cond_insert_point, g, GSI_NEW_STMT);
	  *gsi = gsi_after_labels (then2_bb);
	}

      /* Generate __ubsan_handle_type_mismatch call.  */
      if (flag_sanitize_trap & SANITIZE_OBJECT_SIZE)
	g = gimple_build_call (builtin_decl_explicit (BUILT_IN_TRAP), 0);
      else
	{
	  tree data
	    = ubsan_create_data ("__ubsan_objsz_data", 1, &loc,
				 ubsan_type_descriptor (TREE_TYPE (ptr),
							UBSAN_PRINT_POINTER),
				 NULL_TREE,
				 build_zero_cst (unsigned_char_type_node),
				 ckind,
				 NULL_TREE);
	  data = build_fold_addr_expr_loc (loc, data);
	  enum built_in_function bcode
	    = (flag_sanitize_recover & SANITIZE_OBJECT_SIZE)
	      ? BUILT_IN_UBSAN_HANDLE_TYPE_MISMATCH_V1
	      : BUILT_IN_UBSAN_HANDLE_TYPE_MISMATCH_V1_ABORT;
	  tree p = make_ssa_name (pointer_sized_int_node);
	  g = gimple_build_assign (p, NOP_EXPR, ptr);
	  gimple_set_location (g, loc);
	  gsi_insert_before (gsi, g, GSI_SAME_STMT);
	  g = gimple_build_call (builtin_decl_explicit (bcode), 2, data, p);
	}
      gimple_set_location (g, loc);
      gsi_insert_before (gsi, g, GSI_SAME_STMT);

      /* Point GSI to next logical statement.  */
      *gsi = gsi_start_bb (fallthru_bb);

      /* Get rid of the UBSAN_OBJECT_SIZE call from the IR.  */
      unlink_stmt_vdef (stmt);
      gsi_remove (&gsi_orig, true);
      return true;
    }

  /* Get rid of the UBSAN_OBJECT_SIZE call from the IR.  */
  unlink_stmt_vdef (stmt);
  gsi_remove (gsi, true);
  return true;
}

/* Expand UBSAN_PTR internal call.  */

bool
ubsan_expand_ptr_ifn (gimple_stmt_iterator *gsip)
{
  gimple_stmt_iterator gsi = *gsip;
  gimple *stmt = gsi_stmt (gsi);
  location_t loc = gimple_location (stmt);
  gcc_assert (gimple_call_num_args (stmt) == 2);
  tree ptr = gimple_call_arg (stmt, 0);
  tree off = gimple_call_arg (stmt, 1);

  if (integer_zerop (off))
    {
      gsi_remove (gsip, true);
      unlink_stmt_vdef (stmt);
      return true;
    }

  basic_block cur_bb = gsi_bb (gsi);
  tree ptrplusoff = make_ssa_name (pointer_sized_int_node);
  tree ptri = make_ssa_name (pointer_sized_int_node);
  int pos_neg = get_range_pos_neg (off);

  /* Split the original block holding the pointer dereference.  */
  edge e = split_block (cur_bb, stmt);

  /* Get a hold on the 'condition block', the 'then block' and the
     'else block'.  */
  basic_block cond_bb = e->src;
  basic_block fallthru_bb = e->dest;
  basic_block then_bb = create_empty_bb (cond_bb);
  basic_block cond_pos_bb = NULL, cond_neg_bb = NULL;
  add_bb_to_loop (then_bb, cond_bb->loop_father);
  loops_state_set (LOOPS_NEED_FIXUP);

  /* Set up the fallthrough basic block.  */
  e->flags = EDGE_FALSE_VALUE;
  if (pos_neg != 3)
    {
      e->probability = profile_probability::very_likely ();

      /* Connect 'then block' with the 'else block'.  This is needed
	 as the ubsan routines we call in the 'then block' are not noreturn.
	 The 'then block' only has one outcoming edge.  */
      make_single_succ_edge (then_bb, fallthru_bb, EDGE_FALLTHRU);

      /* Make an edge coming from the 'cond block' into the 'then block';
	 this edge is unlikely taken, so set up the probability
	 accordingly.  */
      e = make_edge (cond_bb, then_bb, EDGE_TRUE_VALUE);
      e->probability = profile_probability::very_unlikely ();
      then_bb->count = e->count ();
    }
  else
    {
      e->probability = profile_probability::even ();

      e = split_block (fallthru_bb, (gimple *) NULL);
      cond_neg_bb = e->src;
      fallthru_bb = e->dest;
      e->probability = profile_probability::very_likely ();
      e->flags = EDGE_FALSE_VALUE;

      e = make_edge (cond_neg_bb, then_bb, EDGE_TRUE_VALUE);
      e->probability = profile_probability::very_unlikely ();
      then_bb->count = e->count ();

      cond_pos_bb = create_empty_bb (cond_bb);
      add_bb_to_loop (cond_pos_bb, cond_bb->loop_father);

      e = make_edge (cond_bb, cond_pos_bb, EDGE_TRUE_VALUE);
      e->probability = profile_probability::even ();
      cond_pos_bb->count = e->count ();

      e = make_edge (cond_pos_bb, then_bb, EDGE_TRUE_VALUE);
      e->probability = profile_probability::very_unlikely ();

      e = make_edge (cond_pos_bb, fallthru_bb, EDGE_FALSE_VALUE);
      e->probability = profile_probability::very_likely ();

      make_single_succ_edge (then_bb, fallthru_bb, EDGE_FALLTHRU);
    }

  gimple *g = gimple_build_assign (ptri, NOP_EXPR, ptr);
  gimple_set_location (g, loc);
  gsi_insert_before (&gsi, g, GSI_SAME_STMT);
  g = gimple_build_assign (ptrplusoff, PLUS_EXPR, ptri, off);
  gimple_set_location (g, loc);
  gsi_insert_before (&gsi, g, GSI_SAME_STMT);

  /* Update dominance info for the newly created then_bb; note that
     fallthru_bb's dominance info has already been updated by
     split_block.  */
  if (dom_info_available_p (CDI_DOMINATORS))
    {
      set_immediate_dominator (CDI_DOMINATORS, then_bb, cond_bb);
      if (pos_neg == 3)
	{
	  set_immediate_dominator (CDI_DOMINATORS, cond_pos_bb, cond_bb);
	  set_immediate_dominator (CDI_DOMINATORS, fallthru_bb, cond_bb);
	}
    }

  /* Put the ubsan builtin call into the newly created BB.  */
  if (flag_sanitize_trap & SANITIZE_POINTER_OVERFLOW)
    g = gimple_build_call (builtin_decl_implicit (BUILT_IN_TRAP), 0);
  else
    {
      enum built_in_function bcode
	= (flag_sanitize_recover & SANITIZE_POINTER_OVERFLOW)
	  ? BUILT_IN_UBSAN_HANDLE_POINTER_OVERFLOW
	  : BUILT_IN_UBSAN_HANDLE_POINTER_OVERFLOW_ABORT;
      tree fn = builtin_decl_implicit (bcode);
      tree data
	= ubsan_create_data ("__ubsan_ptrovf_data", 1, &loc,
			     NULL_TREE, NULL_TREE);
      data = build_fold_addr_expr_loc (loc, data);
      g = gimple_build_call (fn, 3, data, ptr, ptrplusoff);
    }
  gimple_stmt_iterator gsi2 = gsi_start_bb (then_bb);
  gimple_set_location (g, loc);
  gsi_insert_after (&gsi2, g, GSI_NEW_STMT);

  /* Unlink the UBSAN_PTRs vops before replacing it.  */
  unlink_stmt_vdef (stmt);

  if (TREE_CODE (off) == INTEGER_CST)
    g = gimple_build_cond (wi::neg_p (wi::to_wide (off)) ? LT_EXPR : GE_EXPR,
			   ptri, fold_build1 (NEGATE_EXPR, sizetype, off),
			   NULL_TREE, NULL_TREE);
  else if (pos_neg != 3)
    g = gimple_build_cond (pos_neg == 1 ? LT_EXPR : GT_EXPR,
			   ptrplusoff, ptri, NULL_TREE, NULL_TREE);
  else
    {
      gsi2 = gsi_start_bb (cond_pos_bb);
      g = gimple_build_cond (LT_EXPR, ptrplusoff, ptri, NULL_TREE, NULL_TREE);
      gimple_set_location (g, loc);
      gsi_insert_after (&gsi2, g, GSI_NEW_STMT);

      gsi2 = gsi_start_bb (cond_neg_bb);
      g = gimple_build_cond (GT_EXPR, ptrplusoff, ptri, NULL_TREE, NULL_TREE);
      gimple_set_location (g, loc);
      gsi_insert_after (&gsi2, g, GSI_NEW_STMT);

      tree t = gimple_build (&gsi, true, GSI_SAME_STMT,
			     loc, NOP_EXPR, ssizetype, off);
      g = gimple_build_cond (GE_EXPR, t, ssize_int (0),
			     NULL_TREE, NULL_TREE);
    }
  gimple_set_location (g, loc);
  /* Replace the UBSAN_PTR with a GIMPLE_COND stmt.  */
  gsi_replace (&gsi, g, false);
  return false;
}


/* Cached __ubsan_vptr_type_cache decl.  */
static GTY(()) tree ubsan_vptr_type_cache_decl;

/* Expand UBSAN_VPTR internal call.  The type is kept on the ckind
   argument which is a constant, because the middle-end treats pointer
   conversions as useless and therefore the type of the first argument
   could be changed to any other pointer type.  */

bool
ubsan_expand_vptr_ifn (gimple_stmt_iterator *gsip)
{
  gimple_stmt_iterator gsi = *gsip;
  gimple *stmt = gsi_stmt (gsi);
  location_t loc = gimple_location (stmt);
  gcc_assert (gimple_call_num_args (stmt) == 5);
  tree op = gimple_call_arg (stmt, 0);
  tree vptr = gimple_call_arg (stmt, 1);
  tree str_hash = gimple_call_arg (stmt, 2);
  tree ti_decl_addr = gimple_call_arg (stmt, 3);
  tree ckind_tree = gimple_call_arg (stmt, 4);
  ubsan_null_ckind ckind = (ubsan_null_ckind) tree_to_uhwi (ckind_tree);
  tree type = TREE_TYPE (TREE_TYPE (ckind_tree));
  gimple *g;
  basic_block fallthru_bb = NULL;

  if (ckind == UBSAN_DOWNCAST_POINTER)
    {
      /* Guard everything with if (op != NULL) { ... }.  */
      basic_block then_bb;
      gimple_stmt_iterator cond_insert_point
	= create_cond_insert_point (gsip, false, false, true,
				    &then_bb, &fallthru_bb);
      g = gimple_build_cond (NE_EXPR, op, build_zero_cst (TREE_TYPE (op)),
			     NULL_TREE, NULL_TREE);
      gimple_set_location (g, loc);
      gsi_insert_after (&cond_insert_point, g, GSI_NEW_STMT);
      *gsip = gsi_after_labels (then_bb);
      gsi_remove (&gsi, false);
      gsi_insert_before (gsip, stmt, GSI_NEW_STMT);
      gsi = *gsip;
    }

  tree htype = TREE_TYPE (str_hash);
  tree cst = wide_int_to_tree (htype,
			       wi::uhwi (((uint64_t) 0x9ddfea08 << 32)
			       | 0xeb382d69, 64));
  g = gimple_build_assign (make_ssa_name (htype), BIT_XOR_EXPR,
			   vptr, str_hash);
  gimple_set_location (g, loc);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);
  g = gimple_build_assign (make_ssa_name (htype), MULT_EXPR,
			   gimple_assign_lhs (g), cst);
  gimple_set_location (g, loc);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);
  tree t1 = gimple_assign_lhs (g);
  g = gimple_build_assign (make_ssa_name (htype), LSHIFT_EXPR,
			   t1, build_int_cst (integer_type_node, 47));
  gimple_set_location (g, loc);
  tree t2 = gimple_assign_lhs (g);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);
  g = gimple_build_assign (make_ssa_name (htype), BIT_XOR_EXPR,
			   vptr, t1);
  gimple_set_location (g, loc);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);
  g = gimple_build_assign (make_ssa_name (htype), BIT_XOR_EXPR,
			   t2, gimple_assign_lhs (g));
  gimple_set_location (g, loc);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);
  g = gimple_build_assign (make_ssa_name (htype), MULT_EXPR,
			   gimple_assign_lhs (g), cst);
  gimple_set_location (g, loc);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);
  tree t3 = gimple_assign_lhs (g);
  g = gimple_build_assign (make_ssa_name (htype), LSHIFT_EXPR,
			   t3, build_int_cst (integer_type_node, 47));
  gimple_set_location (g, loc);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);
  g = gimple_build_assign (make_ssa_name (htype), BIT_XOR_EXPR,
			   t3, gimple_assign_lhs (g));
  gimple_set_location (g, loc);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);
  g = gimple_build_assign (make_ssa_name (htype), MULT_EXPR,
			   gimple_assign_lhs (g), cst);
  gimple_set_location (g, loc);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);
  if (!useless_type_conversion_p (pointer_sized_int_node, htype))
    {
      g = gimple_build_assign (make_ssa_name (pointer_sized_int_node),
			       NOP_EXPR, gimple_assign_lhs (g));
      gimple_set_location (g, loc);
      gsi_insert_before (gsip, g, GSI_SAME_STMT);
    }
  tree hash = gimple_assign_lhs (g);

  if (ubsan_vptr_type_cache_decl == NULL_TREE)
    {
      tree atype = build_array_type_nelts (pointer_sized_int_node, 128);
      tree array = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			       get_identifier ("__ubsan_vptr_type_cache"),
			       atype);
      DECL_ARTIFICIAL (array) = 1;
      DECL_IGNORED_P (array) = 1;
      TREE_PUBLIC (array) = 1;
      TREE_STATIC (array) = 1;
      DECL_EXTERNAL (array) = 1;
      DECL_VISIBILITY (array) = VISIBILITY_DEFAULT;
      DECL_VISIBILITY_SPECIFIED (array) = 1;
      varpool_node::finalize_decl (array);
      ubsan_vptr_type_cache_decl = array;
   }

  g = gimple_build_assign (make_ssa_name (pointer_sized_int_node),
			   BIT_AND_EXPR, hash,
			   build_int_cst (pointer_sized_int_node, 127));
  gimple_set_location (g, loc);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);

  tree c = build4_loc (loc, ARRAY_REF, pointer_sized_int_node,
		       ubsan_vptr_type_cache_decl, gimple_assign_lhs (g),
		       NULL_TREE, NULL_TREE);
  g = gimple_build_assign (make_ssa_name (pointer_sized_int_node),
			   ARRAY_REF, c);
  gimple_set_location (g, loc);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);

  basic_block then_bb, fallthru2_bb;
  gimple_stmt_iterator cond_insert_point
    = create_cond_insert_point (gsip, false, false, true,
				&then_bb, &fallthru2_bb);
  g = gimple_build_cond (NE_EXPR, gimple_assign_lhs (g), hash,
			 NULL_TREE, NULL_TREE);
  gimple_set_location (g, loc);
  gsi_insert_after (&cond_insert_point, g, GSI_NEW_STMT);
  *gsip = gsi_after_labels (then_bb);
  if (fallthru_bb == NULL)
    fallthru_bb = fallthru2_bb;

  tree data
    = ubsan_create_data ("__ubsan_vptr_data", 1, &loc,
			 ubsan_type_descriptor (type), NULL_TREE, ti_decl_addr,
			 build_int_cst (unsigned_char_type_node, ckind),
			 NULL_TREE);
  data = build_fold_addr_expr_loc (loc, data);
  enum built_in_function bcode
    = (flag_sanitize_recover & SANITIZE_VPTR)
      ? BUILT_IN_UBSAN_HANDLE_DYNAMIC_TYPE_CACHE_MISS
      : BUILT_IN_UBSAN_HANDLE_DYNAMIC_TYPE_CACHE_MISS_ABORT;

  g = gimple_build_call (builtin_decl_explicit (bcode), 3, data, op, hash);
  gimple_set_location (g, loc);
  gsi_insert_before (gsip, g, GSI_SAME_STMT);

  /* Point GSI to next logical statement.  */
  *gsip = gsi_start_bb (fallthru_bb);

  /* Get rid of the UBSAN_VPTR call from the IR.  */
  unlink_stmt_vdef (stmt);
  gsi_remove (&gsi, true);
  return true;
}

/* Instrument a memory reference.  BASE is the base of MEM, IS_LHS says
   whether the pointer is on the left hand side of the assignment.  */

static void
instrument_mem_ref (tree mem, tree base, gimple_stmt_iterator *iter,
		    bool is_lhs)
{
  enum ubsan_null_ckind ikind = is_lhs ? UBSAN_STORE_OF : UBSAN_LOAD_OF;
  location_t site_loc = gimple_location (gsi_stmt (*iter));
  /* A P3100 ub:expr.unary.dereference.nullptr assertion also wants the null
     check on this dereference, even when -fsanitize=null is not enabled.
     Resolve the reaction HERE, at pass_ubsan (pre-inline), where cfun->decl is
     the true enclosing function whose namespace the contract config matches.
     The value is carried as the 4th operand on IFN_UBSAN_NULL so it survives
     inlining and LTO -- ubsan_expand_null_ifn (post-inline) must NOT re-resolve
     it, since cfun->decl there may be a caller in a different namespace.  When
     no contract applies the reaction is IMPLICIT_UB_NONE (0), which every
     consumer treats exactly as the old "no contract, sanitizer only" path.  */
  int p3100_reaction = implicit_null_deref_reaction (site_loc);
  bool p3100_null = (p3100_reaction != IMPLICIT_UB_NONE);
  /* A P3100 ub:basic.align.object.alignment assertion wants the alignment edge
     generated even when -fsanitize=alignment is not enabled.  Its reaction is
     carried independently (operands 6/7/8), and ubsan_expand_null_ifn routes the
     null and alignment edges to separate then-blocks, so both a null-deref
     contract and an alignment contract can apply at the same access.  */
  int p3100_align_reaction = implicit_align_reaction (site_loc);
  bool want_align_contract = (p3100_align_reaction != IMPLICIT_UB_NONE);
  unsigned int align = 0;
  if (sanitize_flags_p (SANITIZE_ALIGNMENT) || want_align_contract)
    {
      align = min_align_of_type (TREE_TYPE (base));
      if (align <= 1)
	align = 0;
    }
  /* Only a type that actually needs alignment (> 1) has an alignment edge; if
     none, the align contract has nothing to check.  This differs from
     want_align_contract, which is true whenever a config selects an alignment
     reaction regardless of whether this access has an alignment edge.  */
  bool p3100_align_active = (want_align_contract && align != 0);
  if (align == 0)
    {
      if (!sanitize_flags_p (SANITIZE_NULL) && !p3100_null)
	return;
      addr_space_t as = TYPE_ADDR_SPACE (TREE_TYPE (base));
      if (!ADDR_SPACE_GENERIC_P (as)
	  && targetm.addr_space.zero_address_valid (as))
	return;
    }
  tree t = TREE_OPERAND (base, 0);
  if (!POINTER_TYPE_P (TREE_TYPE (t)))
    return;
  if (RECORD_OR_UNION_TYPE_P (TREE_TYPE (base)) && mem != base)
    ikind = UBSAN_MEMBER_ACCESS;
  tree kind = build_int_cst (build_pointer_type (TREE_TYPE (base)), ikind);
  tree alignt = build_int_cst (pointer_sized_int_node, align);
  tree reactiont = build_int_cst (unsigned_type_node, p3100_reaction);
  tree align_reactiont
    = build_int_cst (unsigned_type_node,
		     p3100_align_active ? p3100_align_reaction
					: IMPLICIT_UB_NONE);
  /* For a P3100 noexcept_enforce/observe check, build the contract violation
     handler entry point and its static data block HERE, at pass_ubsan, while
     the C++ front-end langhook is still available and cfun->decl is the true
     (pre-inline) enclosing function.  The handler entry decl and data-block
     address are carried on the IFN so ubsan_expand_null_ifn (which runs at
     sanopt, post-inline and -- crucially -- at LTRANS where no C++ langhook
     exists) can emit the call by reading the IR instead of re-deriving it.
     quick_enforce (trap) and the pure-sanitizer path carry null operands and
     fall back to a trap / the sanitizer handler at expansion time.  The null
     reaction uses operands 4/5; the alignment reaction operands 7/8.  */
  tree entryt = null_pointer_node;
  tree data_addrt = null_pointer_node;
  if (p3100_reaction == IMPLICIT_UB_NOEXCEPT_ENFORCE
      || p3100_reaction == IMPLICIT_UB_NOEXCEPT_OBSERVE)
    {
      tree entry = NULL_TREE, data_addr = NULL_TREE;
      if (lang_hooks.build_implicit_ub_handler
	    (cfun->decl, site_loc,
	     "ub:expr.unary.dereference.nullptr", p3100_reaction,
	     &entry, &data_addr))
	{
	  entryt = build_fold_addr_expr (entry);
	  data_addrt = data_addr;
	}
    }
  tree align_entryt = null_pointer_node;
  tree align_data_addrt = null_pointer_node;
  if (p3100_align_active
      && (p3100_align_reaction == IMPLICIT_UB_NOEXCEPT_ENFORCE
	  || p3100_align_reaction == IMPLICIT_UB_NOEXCEPT_OBSERVE))
    {
      tree entry = NULL_TREE, data_addr = NULL_TREE;
      if (lang_hooks.build_implicit_ub_handler
	    (cfun->decl, site_loc,
	     "ub:basic.align.object.alignment", p3100_align_reaction,
	     &entry, &data_addr))
	{
	  align_entryt = build_fold_addr_expr (entry);
	  align_data_addrt = data_addr;
	}
    }
  gcall *g = gimple_build_call_internal (IFN_UBSAN_NULL, UBSAN_NULL_NUM_OPS,
					 t, kind, alignt, reactiont, entryt,
					 data_addrt, align_reactiont,
					 align_entryt, align_data_addrt);
  gimple_set_location (g, site_loc);
  gsi_safe_insert_before (iter, g);
}

/* Perform the pointer instrumentation.  */

static void
instrument_null (gimple_stmt_iterator gsi, tree t, bool is_lhs)
{
  /* Handle also e.g. &s->i.  */
  if (TREE_CODE (t) == ADDR_EXPR)
    t = TREE_OPERAND (t, 0);
  tree base = get_base_address (t);
  if (base != NULL_TREE
      && TREE_CODE (base) == MEM_REF
      && TREE_CODE (TREE_OPERAND (base, 0)) == SSA_NAME)
    instrument_mem_ref (t, base, &gsi, is_lhs);
}

/* Instrument pointer arithmetics PTR p+ OFF.  */

static void
instrument_pointer_overflow (gimple_stmt_iterator *gsi, tree ptr, tree off)
{
  if (TYPE_PRECISION (sizetype) != POINTER_SIZE)
    return;
  gcall *g = gimple_build_call_internal (IFN_UBSAN_PTR, 2, ptr, off);
  gimple_set_location (g, gimple_location (gsi_stmt (*gsi)));
  gsi_safe_insert_before (gsi, g);
}

/* Instrument pointer arithmetics if any.  */

static void
maybe_instrument_pointer_overflow (gimple_stmt_iterator *gsi, tree t)
{
  if (TYPE_PRECISION (sizetype) != POINTER_SIZE)
    return;

  /* Handle also e.g. &s->i.  */
  if (TREE_CODE (t) == ADDR_EXPR)
    t = TREE_OPERAND (t, 0);

  if (!handled_component_p (t) && TREE_CODE (t) != MEM_REF)
    return;

  poly_int64 bitsize, bitpos, bytepos;
  tree offset;
  machine_mode mode;
  int volatilep = 0, reversep, unsignedp = 0;
  tree inner = get_inner_reference (t, &bitsize, &bitpos, &offset, &mode,
				    &unsignedp, &reversep, &volatilep);
  tree moff = NULL_TREE;

  bool decl_p = DECL_P (inner);
  tree base;
  if (decl_p)
    {
      if ((VAR_P (inner)
	   || TREE_CODE (inner) == PARM_DECL
	   || TREE_CODE (inner) == RESULT_DECL)
	  && DECL_REGISTER (inner))
	return;
      base = inner;
      /* If BASE is a fixed size automatic variable or
	 global variable defined in the current TU and bitpos
	 fits, don't instrument anything.  */
      poly_int64 base_size;
      if (offset == NULL_TREE
	  && maybe_ne (bitpos, 0)
	  && (VAR_P (base)
	      || TREE_CODE (base) == PARM_DECL
	      || TREE_CODE (base) == RESULT_DECL)
	  && poly_int_tree_p (DECL_SIZE (base), &base_size)
	  && known_ge (base_size, bitpos)
	  && (!is_global_var (base) || decl_binds_to_current_def_p (base)))
	return;
    }
  else if (TREE_CODE (inner) == MEM_REF)
    {
      base = TREE_OPERAND (inner, 0);
      if (TREE_CODE (base) == ADDR_EXPR
	  && DECL_P (TREE_OPERAND (base, 0))
	  && !TREE_ADDRESSABLE (TREE_OPERAND (base, 0))
	  && !is_global_var (TREE_OPERAND (base, 0)))
	return;
      moff = TREE_OPERAND (inner, 1);
      if (integer_zerop (moff))
	moff = NULL_TREE;
    }
  else
    return;

  if (!POINTER_TYPE_P (TREE_TYPE (base)) && !DECL_P (base))
    return;
  bytepos = bits_to_bytes_round_down (bitpos);
  if (offset == NULL_TREE && known_eq (bytepos, 0) && moff == NULL_TREE)
    return;

  tree base_addr = base;
  if (decl_p)
    base_addr = build1 (ADDR_EXPR,
			build_pointer_type (TREE_TYPE (base)), base);
  t = offset;
  if (maybe_ne (bytepos, 0))
    {
      if (t)
	t = fold_build2 (PLUS_EXPR, TREE_TYPE (t), t,
			 build_int_cst (TREE_TYPE (t), bytepos));
      else
	t = size_int (bytepos);
    }
  if (moff)
    {
      if (t)
	t = fold_build2 (PLUS_EXPR, TREE_TYPE (t), t,
			 fold_convert (TREE_TYPE (t), moff));
      else
	t = fold_convert (sizetype, moff);
    }
  gimple_seq seq, this_seq;
  t = force_gimple_operand (t, &seq, true, NULL_TREE);
  base_addr = force_gimple_operand (base_addr, &this_seq, true, NULL_TREE);
  gimple_seq_add_seq_without_update (&seq, this_seq);
  gsi_safe_insert_seq_before (gsi, seq);
  instrument_pointer_overflow (gsi, base_addr, t);
}

/* Build an ubsan builtin call for the signed-integer-overflow
   sanitization.  CODE says what kind of builtin are we building,
   LOC is a location, LHSTYPE is the type of LHS, OP0 and OP1
   are operands of the binary operation.  */

tree
ubsan_build_overflow_builtin (tree_code code, location_t loc, tree lhstype,
			      tree op0, tree op1, tree *datap)
{
  /* P3100 implicit signed-overflow contract assertions do not reach here: they
     are lowered entirely in pass_ubsan (see instrument_si_overflow_contract),
     which runs before inlining so the semantic is resolved once with the
     correct enclosing-function context.  This RTL path is sanitizer-only.  */
  if (flag_sanitize_trap & SANITIZE_SI_OVERFLOW)
    return build_call_expr_loc (loc, builtin_decl_explicit (BUILT_IN_TRAP), 0);

  tree data;
  if (datap && *datap)
    data = *datap;
  else
    data = ubsan_create_data ("__ubsan_overflow_data", 1, &loc,
			      ubsan_type_descriptor (lhstype), NULL_TREE,
			      NULL_TREE);
  if (datap)
    *datap = data;
  enum built_in_function fn_code;

  switch (code)
    {
    case PLUS_EXPR:
      fn_code = (flag_sanitize_recover & SANITIZE_SI_OVERFLOW)
		? BUILT_IN_UBSAN_HANDLE_ADD_OVERFLOW
		: BUILT_IN_UBSAN_HANDLE_ADD_OVERFLOW_ABORT;
      break;
    case MINUS_EXPR:
      fn_code = (flag_sanitize_recover & SANITIZE_SI_OVERFLOW)
		? BUILT_IN_UBSAN_HANDLE_SUB_OVERFLOW
		: BUILT_IN_UBSAN_HANDLE_SUB_OVERFLOW_ABORT;
      break;
    case MULT_EXPR:
      fn_code = (flag_sanitize_recover & SANITIZE_SI_OVERFLOW)
		? BUILT_IN_UBSAN_HANDLE_MUL_OVERFLOW
		: BUILT_IN_UBSAN_HANDLE_MUL_OVERFLOW_ABORT;
      break;
    case NEGATE_EXPR:
      fn_code = (flag_sanitize_recover & SANITIZE_SI_OVERFLOW)
		? BUILT_IN_UBSAN_HANDLE_NEGATE_OVERFLOW
		: BUILT_IN_UBSAN_HANDLE_NEGATE_OVERFLOW_ABORT;
      break;
    default:
      gcc_unreachable ();
    }
  tree fn = builtin_decl_explicit (fn_code);
  return build_call_expr_loc (loc, fn, 2 + (code != NEGATE_EXPR),
			      build_fold_addr_expr_loc (loc, data),
			      ubsan_encode_value (op0, UBSAN_ENCODE_VALUE_RTL),
			      op1
			      ? ubsan_encode_value (op1,
						    UBSAN_ENCODE_VALUE_RTL)
			      : NULL_TREE);
}

/* P3100: lower an implicit signed-overflow contract assertion at the statement
   *GSI points to.  REACTION is the language-neutral reaction already resolved
   (once, here in pass_ubsan, with the correct enclosing-function context, so
   inlining can never change it).  STMT is a signed PLUS/MINUS/MULT/NEGATE assign
   that may overflow.  We rewrite it into a .{ADD,SUB,MUL}_OVERFLOW internal call
   whose real part is the defined 2's-complement wrapped result (used
   unconditionally) and, unless REACTION is IMPLICIT_UB_DEFINED (ignore), a
   very-unlikely branch on the overflow flag to the reaction: a trap for
   quick_enforce, or the nothrow contract handler for noexcept_enforce/observe
   (enforce is noreturn; observe returns and continues with the wrapped result).
   Using an internal function for the operation also stops the optimizer
   assuming it cannot overflow, so loops built on it are no longer treated as
   provably finite -- exactly the codegen change ignore/observe must make.  On
   return *GSI points at the (new) statement defining LHS, in the original block,
   so the caller's per-bb loop resumes cleanly.  */

static void
instrument_si_overflow_contract (gimple_stmt_iterator *gsi, int reaction)
{
  gimple *stmt = gsi_stmt (*gsi);
  tree_code code = gimple_assign_rhs_code (stmt);
  tree lhs = gimple_assign_lhs (stmt);
  tree type = TREE_TYPE (lhs);
  location_t loc = gimple_location (stmt);
  internal_fn ifn;
  tree a, b;

  switch (code)
    {
    case PLUS_EXPR:
      ifn = IFN_ADD_OVERFLOW;
      a = gimple_assign_rhs1 (stmt); b = gimple_assign_rhs2 (stmt);
      break;
    case MINUS_EXPR:
      ifn = IFN_SUB_OVERFLOW;
      a = gimple_assign_rhs1 (stmt); b = gimple_assign_rhs2 (stmt);
      break;
    case MULT_EXPR:
      ifn = IFN_MUL_OVERFLOW;
      a = gimple_assign_rhs1 (stmt); b = gimple_assign_rhs2 (stmt);
      break;
    case NEGATE_EXPR:
      /* -u is 0 - u; .SUB_OVERFLOW (0, u) overflows exactly for u == INT_MIN.  */
      ifn = IFN_SUB_OVERFLOW;
      a = build_zero_cst (type); b = gimple_assign_rhs1 (stmt);
      break;
    default:
      gcc_unreachable ();
    }

  /* res = .{ADD,SUB,MUL}_OVERFLOW (a, b);  complex: real = wrapped result,
     imag = nonzero iff the operation overflowed.  */
  tree ctype = build_complex_type (type);
  gcall *gc = gimple_build_call_internal (ifn, 2, a, b);
  tree res = make_ssa_name (ctype);
  gimple_call_set_lhs (gc, res);
  gimple_set_location (gc, loc);
  gsi_insert_before (gsi, gc, GSI_SAME_STMT);

  /* lhs = REALPART_EXPR <res>;  the defined wrapped result, used regardless of
     whether the operation overflowed.  Replaces the original assignment.  */
  gassign *gr = gimple_build_assign (lhs, REALPART_EXPR,
				     build1 (REALPART_EXPR, type, res));
  gimple_set_location (gr, loc);
  gsi_replace (gsi, gr, true);

  if (reaction == IMPLICIT_UB_DEFINED)
    /* ignore: the wrapped result, no reaction and no branch.  */
    return;

  /* ovf = IMAGPART_EXPR <res>;  */
  tree ovf = make_ssa_name (type);
  gassign *gi = gimple_build_assign (ovf, IMAGPART_EXPR,
				     build1 (IMAGPART_EXPR, type, res));
  gimple_set_location (gi, loc);
  gsi_insert_after (gsi, gi, GSI_NEW_STMT);

  /* if (ovf != 0) <reaction>;  */
  basic_block then_bb, fallthru_bb;
  gimple_stmt_iterator cond_gsi
    = create_cond_insert_point (gsi, /*before_p=*/false,
				/*then_more_likely_p=*/false,
				/*create_then_fallthru_edge=*/true,
				&then_bb, &fallthru_bb);
  gcond *cond = gimple_build_cond (NE_EXPR, ovf, build_zero_cst (type),
				   NULL_TREE, NULL_TREE);
  gimple_set_location (cond, loc);
  gsi_insert_after (&cond_gsi, cond, GSI_NEW_STMT);

  gimple *g;
  if (reaction == IMPLICIT_UB_NOEXCEPT_ENFORCE
      || reaction == IMPLICIT_UB_NOEXCEPT_OBSERVE)
    {
      tree entry = NULL_TREE, data_addr = NULL_TREE;
      if (lang_hooks.build_implicit_ub_handler (cfun->decl, loc,
						"ub:expr.expr.eval.signed.integer",
						reaction, &entry, &data_addr))
	/* noexcept_enforce (the decl encodes noreturn) / noexcept_observe
	   (returns and falls through to the wrapped result).  Throwing
	   enforce/observe are excluded from this check's allowed set on the
	   front-end side and clamped away, so they never reach here.  */
	g = gimple_build_call (entry, 1, data_addr);
      else
	g = gimple_build_call (builtin_decl_implicit (BUILT_IN_TRAP), 0);
    }
  else
    /* IMPLICIT_UB_TRAP (quick_enforce).  */
    g = gimple_build_call (builtin_decl_implicit (BUILT_IN_TRAP), 0);
  gimple_stmt_iterator then_gsi = gsi_after_labels (then_bb);
  gimple_set_location (g, loc);
  gsi_insert_before (&then_gsi, g, GSI_SAME_STMT);

  /* Resume the caller's per-bb walk at LHS's definition, which stays in the
     original (now condition) block; the new then/fallthru blocks are visited
     in turn by the pass's FOR_EACH_BB loop.  */
  *gsi = gsi_for_stmt (gr);
}

/* Perform the signed integer instrumentation.  *GSI is the iterator pointing at
   the statement we are trying to instrument.  */

static void
instrument_si_overflow (gimple_stmt_iterator *gsi)
{
  gimple *stmt = gsi_stmt (*gsi);
  tree_code code = gimple_assign_rhs_code (stmt);
  tree lhs = gimple_assign_lhs (stmt);
  tree lhstype = TREE_TYPE (lhs);
  tree lhsinner = VECTOR_TYPE_P (lhstype) ? TREE_TYPE (lhstype) : lhstype;
  tree a, b;
  gimple *g;

  /* If this is not a signed operation, don't instrument anything here.
     Also punt on bit-fields.  */
  if (!INTEGRAL_TYPE_P (lhsinner)
      || TYPE_OVERFLOW_WRAPS (lhsinner)
      || (!BITINT_TYPE_P (lhsinner)
	  && maybe_ne (GET_MODE_BITSIZE (TYPE_MODE (lhsinner)),
		       TYPE_PRECISION (lhsinner))))
    return;

  /* When the sanitizer is off, only instrument for a P3100 implicit
     signed-overflow contract assertion.  A vector operation has no scalar
     overflow site to check, and ABS is not handled by the contract lowering, so
     both fall through to assume.  The site is resolved once here (pre-inline);
     assume leaves the raw operation (optimized, byte-identical), any other
     semantic is lowered by instrument_si_overflow_contract.  */
  if (!sanitize_flags_p (SANITIZE_SI_OVERFLOW))
    {
      if (!flag_contracts_p3100
	  || VECTOR_TYPE_P (lhstype)
	  || BITINT_TYPE_P (lhsinner))
	return;
      if (code != PLUS_EXPR && code != MINUS_EXPR && code != MULT_EXPR
	  && code != NEGATE_EXPR)
	return;
      int reaction = implicit_overflow_reaction (gimple_location (stmt));
      if (reaction == IMPLICIT_UB_NONE)
	return;
      instrument_si_overflow_contract (gsi, reaction);
      return;
    }

  switch (code)
    {
    case MINUS_EXPR:
    case PLUS_EXPR:
    case MULT_EXPR:
      /* Transform
	 i = u {+,-,*} 5;
	 into
	 i = UBSAN_CHECK_{ADD,SUB,MUL} (u, 5);  */
      a = gimple_assign_rhs1 (stmt);
      b = gimple_assign_rhs2 (stmt);
      g = gimple_build_call_internal (code == PLUS_EXPR
				      ? IFN_UBSAN_CHECK_ADD
				      : code == MINUS_EXPR
				      ? IFN_UBSAN_CHECK_SUB
				      : IFN_UBSAN_CHECK_MUL, 2, a, b);
      gimple_call_set_lhs (g, lhs);
      gsi_replace (gsi, g, true);
      break;
    case NEGATE_EXPR:
      /* Represent i = -u;
	 as
	 i = UBSAN_CHECK_SUB (0, u);  */
      a = build_zero_cst (lhstype);
      b = gimple_assign_rhs1 (stmt);
      g = gimple_build_call_internal (IFN_UBSAN_CHECK_SUB, 2, a, b);
      gimple_call_set_lhs (g, lhs);
      gsi_replace (gsi, g, true);
      break;
    case ABS_EXPR:
      /* Transform i = ABS_EXPR<u>;
	 into
	 _N = UBSAN_CHECK_SUB (0, u);
	 i = ABS_EXPR<_N>;  */
      a = build_zero_cst (lhstype);
      b = gimple_assign_rhs1 (stmt);
      g = gimple_build_call_internal (IFN_UBSAN_CHECK_SUB, 2, a, b);
      a = make_ssa_name (lhstype);
      gimple_call_set_lhs (g, a);
      gimple_set_location (g, gimple_location (stmt));
      gsi_insert_before (gsi, g, GSI_SAME_STMT);
      gimple_assign_set_rhs1 (stmt, a);
      update_stmt (stmt);
      break;
    default:
      break;
    }
}

/* P3100: lower an implicit invalid-value-load contract assertion at the
   (non-bitfield) bool/enum load STMT points to.  REACTION is the reaction
   already resolved once, here in pass_ubsan, with the correct enclosing-function
   context.  Reuses the sanitizer's raw-bits load + range test
   (instrument_bool_enum_load), but instead of "report then continue with the
   raw value" it *substitutes a defined valid value* (0 -- false for bool, an
   in-range value for enum) whenever the stored value is out of range.  The
   substitution is unconditional -- LHS = (out_of_range ? 0 : raw) -- so the
   defined value is produced without a PHI (like instrument_si_overflow_contract
   for overflow); for a checking reaction an extra very-unlikely branch runs the
   trap / nothrow handler for its side effect only.  ignore
   (IMPLICIT_UB_DEFINED) is just the substitution, no branch.  On return *GSI
   points at the (new) statement defining LHS, in the original block.  */

static void
instrument_bool_enum_load_contract (gimple_stmt_iterator *gsi, int reaction)
{
  gimple *stmt = gsi_stmt (*gsi);
  tree rhs = gimple_assign_rhs1 (stmt);
  tree type = TREE_TYPE (rhs);
  tree lhs = gimple_assign_lhs (stmt);
  tree minv = NULL_TREE, maxv = NULL_TREE;

  if (TREE_CODE (type) == BOOLEAN_TYPE)
    {
      minv = boolean_false_node;
      maxv = boolean_true_node;
    }
  else if (TREE_CODE (type) == ENUMERAL_TYPE
	   && TREE_TYPE (type) != NULL_TREE
	   && TREE_CODE (TREE_TYPE (type)) == INTEGER_TYPE
	   && (TYPE_PRECISION (TREE_TYPE (type))
	       < GET_MODE_PRECISION (SCALAR_INT_TYPE_MODE (type))))
    {
      minv = TYPE_MIN_VALUE (TREE_TYPE (type));
      maxv = TYPE_MAX_VALUE (TREE_TYPE (type));
    }
  else
    return;

  int modebitsize = GET_MODE_BITSIZE (SCALAR_INT_TYPE_MODE (type));
  poly_int64 bitsize, bitpos;
  tree offset;
  machine_mode mode;
  int volatilep = 0, reversep, unsignedp = 0;
  tree base = get_inner_reference (rhs, &bitsize, &bitpos, &offset, &mode,
				   &unsignedp, &reversep, &volatilep);
  tree utype = build_nonstandard_integer_type (modebitsize, 1);

  /* Same eligibility as the sanitizer.  */
  if ((VAR_P (base) && DECL_HARD_REGISTER (base))
      || !multiple_p (bitpos, modebitsize)
      || maybe_ne (bitsize, modebitsize)
      || GET_MODE_BITSIZE (SCALAR_INT_TYPE_MODE (utype)) != modebitsize
      || TREE_CODE (lhs) != SSA_NAME)
    return;

  /* A load that ends its block -- it can throw, which under
     -fnon-call-exceptions is the case for any load in an EH region, and an
     EH region is created by something as ordinary as a local with a
     destructor.  We can neither insert after it nor replace it, so
     retarget the load itself to produce the raw bits and build the check
     and the value substitution on the fallthrough edge, exactly as the
     stock instrument_bool_enum_load does.  Bailing here instead would
     leave the raw load in place -- that is, behave as assume -- whatever
     semantic was actually configured, including the value substitution
     that ignore requires.  */
  bool ends_bb = stmt_ends_bb_p (stmt);

  addr_space_t as = TYPE_ADDR_SPACE (TREE_TYPE (rhs));
  if (as != TYPE_ADDR_SPACE (utype))
    utype = build_qualified_type (utype, TYPE_QUALS (utype)
					 | ENCODE_QUAL_ADDR_SPACE (as));
  location_t loc = gimple_location (stmt);

  /* Load the raw storage bits as an unsigned integer, so an out-of-range value
     is visible (a plain bool/enum load would already be assumed in range).  */
  tree ptype = build_pointer_type (TREE_TYPE (rhs));
  tree atype = reference_alias_ptr_type (rhs);
  gimple *g = gimple_build_assign (make_ssa_name (ptype),
				   build_fold_addr_expr (rhs));
  gimple_set_location (g, loc);
  gsi_insert_before (gsi, g, GSI_SAME_STMT);
  tree mem = build2 (MEM_REF, utype, gimple_assign_lhs (g),
		     build_int_cst (atype, 0));
  tree urhs = make_ssa_name (utype);

  /* INS is where the check and the substitution get built.  RAWVAL is the
     raw value reinterpreted as the bool/enum type, used only on the
     in-range path; it doubles as the anchor statement on the fallthrough
     edge in the ends_bb case.  */
  gimple_stmt_iterator ins;
  tree rawval = make_ssa_name (type);
  if (ends_bb)
    {
      gimple_assign_set_lhs (stmt, urhs);
      gimple_assign_set_rhs_from_tree (gsi, mem);
      update_stmt (stmt);
      edge e = find_fallthru_edge (gimple_bb (stmt)->succs);
      gcc_assert (e != NULL);
      g = gimple_build_assign (rawval, NOP_EXPR, urhs);
      gimple_set_location (g, loc);
      gsi_insert_on_edge_immediate (e, g);
      ins = gsi_for_stmt (g);
    }
  else
    {
      g = gimple_build_assign (urhs, mem);
      gimple_set_location (g, loc);
      gsi_insert_before (gsi, g, GSI_SAME_STMT);
      ins = gsi_for_stmt (g);
      g = gimple_build_assign (rawval, NOP_EXPR, urhs);
      gimple_set_location (g, loc);
      gsi_insert_after (&ins, g, GSI_NEW_STMT);
    }

  /* Normalize to [0, maxv-minv] and form the out-of-range test, exactly as the
     sanitizer does.  */
  tree umin = fold_convert (utype, minv);
  tree umax = fold_convert (utype, maxv);
  tree norm = urhs;
  if (!integer_zerop (umin))
    {
      norm = make_ssa_name (utype);
      g = gimple_build_assign (norm, MINUS_EXPR, urhs, umin);
      gimple_set_location (g, loc);
      gsi_insert_after (&ins, g, GSI_NEW_STMT);
    }
  tree bound = int_const_binop (MINUS_EXPR, umax, umin);

  /* out_of_range = norm > bound.  A GIMPLE COND_EXPR select requires a bare
     boolean condition (not a comparison), so materialize it into an SSA name;
     it is reused for the reaction branch below.  */
  tree ovf = make_ssa_name (boolean_type_node);
  g = gimple_build_assign (ovf, GT_EXPR, norm, bound);
  gimple_set_location (g, loc);
  gsi_insert_after (&ins, g, GSI_NEW_STMT);

  /* LHS = out_of_range ? 0 : rawval -- the defined valid value, produced
     unconditionally.  It replaces the original load, or on the ends_bb path
     defines LHS on the fallthrough edge, the load itself having been
     retargeted to produce the raw bits.  */
  gassign *sel = gimple_build_assign (lhs, COND_EXPR, ovf,
				      build_zero_cst (type), rawval);
  gimple_set_location (sel, loc);
  if (ends_bb)
    gsi_insert_after (&ins, sel, GSI_NEW_STMT);
  else
    gsi_replace (gsi, sel, true);

  if (reaction == IMPLICIT_UB_DEFINED)
    /* ignore: just the substitution, no handler and no branch.  */
    return;

  /* Checking semantic: add a very-unlikely branch on the out-of-range test to
     run the reaction; LHS is already 0 on that path.  */
  basic_block then_bb, fallthru_bb;
  gimple_stmt_iterator sel_gsi = gsi_for_stmt (sel);
  gimple_stmt_iterator cond_gsi
    = create_cond_insert_point (&sel_gsi, /*before_p=*/false,
				/*then_more_likely_p=*/false,
				/*create_then_fallthru_edge=*/true,
				&then_bb, &fallthru_bb);
  gcond *cond = gimple_build_cond (NE_EXPR, ovf, boolean_false_node,
				   NULL_TREE, NULL_TREE);
  gimple_set_location (cond, loc);
  gsi_insert_after (&cond_gsi, cond, GSI_NEW_STMT);

  gimple *r;
  if (reaction == IMPLICIT_UB_NOEXCEPT_ENFORCE
      || reaction == IMPLICIT_UB_NOEXCEPT_OBSERVE)
    {
      tree entry = NULL_TREE, data_addr = NULL_TREE;
      if (lang_hooks.build_implicit_ub_handler
	    (cfun->decl, loc, "ub:conv.lval.valid.representation.bool.enum",
	     reaction, &entry, &data_addr))
	r = gimple_build_call (entry, 1, data_addr);
      else
	r = gimple_build_call (builtin_decl_implicit (BUILT_IN_TRAP), 0);
    }
  else
    /* IMPLICIT_UB_TRAP (quick_enforce).  */
    r = gimple_build_call (builtin_decl_implicit (BUILT_IN_TRAP), 0);
  gimple_stmt_iterator then_gsi = gsi_after_labels (then_bb);
  gimple_set_location (r, loc);
  gsi_insert_before (&then_gsi, r, GSI_SAME_STMT);

  *gsi = gsi_for_stmt (sel);
}

/* Instrument loads from (non-bitfield) bool and C++ enum values
   to check if the memory value is outside of the range of the valid
   type values.  */

static void
instrument_bool_enum_load (gimple_stmt_iterator *gsi)
{
  gimple *stmt = gsi_stmt (*gsi);
  tree rhs = gimple_assign_rhs1 (stmt);
  tree type = TREE_TYPE (rhs);
  tree minv = NULL_TREE, maxv = NULL_TREE;

  if (TREE_CODE (type) == BOOLEAN_TYPE
      && sanitize_flags_p (SANITIZE_BOOL))
    {
      minv = boolean_false_node;
      maxv = boolean_true_node;
    }
  else if (TREE_CODE (type) == ENUMERAL_TYPE
	   && sanitize_flags_p (SANITIZE_ENUM)
	   && TREE_TYPE (type) != NULL_TREE
	   && TREE_CODE (TREE_TYPE (type)) == INTEGER_TYPE
	   && (TYPE_PRECISION (TREE_TYPE (type))
	       < GET_MODE_PRECISION (SCALAR_INT_TYPE_MODE (type))))
    {
      minv = TYPE_MIN_VALUE (TREE_TYPE (type));
      maxv = TYPE_MAX_VALUE (TREE_TYPE (type));
    }
  else
    return;

  int modebitsize = GET_MODE_BITSIZE (SCALAR_INT_TYPE_MODE (type));
  poly_int64 bitsize, bitpos;
  tree offset;
  machine_mode mode;
  int volatilep = 0, reversep, unsignedp = 0;
  tree base = get_inner_reference (rhs, &bitsize, &bitpos, &offset, &mode,
				   &unsignedp, &reversep, &volatilep);
  tree utype = build_nonstandard_integer_type (modebitsize, 1);

  if ((VAR_P (base) && DECL_HARD_REGISTER (base))
      || !multiple_p (bitpos, modebitsize)
      || maybe_ne (bitsize, modebitsize)
      || GET_MODE_BITSIZE (SCALAR_INT_TYPE_MODE (utype)) != modebitsize
      || TREE_CODE (gimple_assign_lhs (stmt)) != SSA_NAME)
    return;

  addr_space_t as = TYPE_ADDR_SPACE (TREE_TYPE (rhs));
  if (as != TYPE_ADDR_SPACE (utype))
    utype = build_qualified_type (utype, TYPE_QUALS (utype)
					 | ENCODE_QUAL_ADDR_SPACE (as));
  bool ends_bb = stmt_ends_bb_p (stmt);
  location_t loc = gimple_location (stmt);
  tree lhs = gimple_assign_lhs (stmt);
  tree ptype = build_pointer_type (TREE_TYPE (rhs));
  tree atype = reference_alias_ptr_type (rhs);
  gimple *g = gimple_build_assign (make_ssa_name (ptype),
				   build_fold_addr_expr (rhs));
  gimple_set_location (g, loc);
  gsi_insert_before (gsi, g, GSI_SAME_STMT);
  tree mem = build2 (MEM_REF, utype, gimple_assign_lhs (g),
		     build_int_cst (atype, 0));
  tree urhs = make_ssa_name (utype);
  if (ends_bb)
    {
      gimple_assign_set_lhs (stmt, urhs);
      g = gimple_build_assign (lhs, NOP_EXPR, urhs);
      gimple_set_location (g, loc);
      edge e = find_fallthru_edge (gimple_bb (stmt)->succs);
      gsi_insert_on_edge_immediate (e, g);
      gimple_assign_set_rhs_from_tree (gsi, mem);
      update_stmt (stmt);
      *gsi = gsi_for_stmt (g);
      g = stmt;
    }
  else
    {
      g = gimple_build_assign (urhs, mem);
      gimple_set_location (g, loc);
      gsi_insert_before (gsi, g, GSI_SAME_STMT);
    }
  minv = fold_convert (utype, minv);
  maxv = fold_convert (utype, maxv);
  if (!integer_zerop (minv))
    {
      g = gimple_build_assign (make_ssa_name (utype), MINUS_EXPR, urhs, minv);
      gimple_set_location (g, loc);
      gsi_insert_before (gsi, g, GSI_SAME_STMT);
    }

  gimple_stmt_iterator gsi2 = *gsi;
  basic_block then_bb, fallthru_bb;
  *gsi = create_cond_insert_point (gsi, true, false, true,
				   &then_bb, &fallthru_bb);
  g = gimple_build_cond (GT_EXPR, gimple_assign_lhs (g),
			 int_const_binop (MINUS_EXPR, maxv, minv),
			 NULL_TREE, NULL_TREE);
  gimple_set_location (g, loc);
  gsi_insert_after (gsi, g, GSI_NEW_STMT);

  if (!ends_bb)
    {
      gimple_assign_set_rhs_with_ops (&gsi2, NOP_EXPR, urhs);
      update_stmt (stmt);
    }

  gsi2 = gsi_after_labels (then_bb);
  if (flag_sanitize_trap & (TREE_CODE (type) == BOOLEAN_TYPE
			    ? SANITIZE_BOOL : SANITIZE_ENUM))
    g = gimple_build_call (builtin_decl_explicit (BUILT_IN_TRAP), 0);
  else
    {
      tree data = ubsan_create_data ("__ubsan_invalid_value_data", 1, &loc,
				     ubsan_type_descriptor (type), NULL_TREE,
				     NULL_TREE);
      data = build_fold_addr_expr_loc (loc, data);
      enum built_in_function bcode
	= (flag_sanitize_recover & (TREE_CODE (type) == BOOLEAN_TYPE
				    ? SANITIZE_BOOL : SANITIZE_ENUM))
	  ? BUILT_IN_UBSAN_HANDLE_LOAD_INVALID_VALUE
	  : BUILT_IN_UBSAN_HANDLE_LOAD_INVALID_VALUE_ABORT;
      tree fn = builtin_decl_explicit (bcode);

      tree val = ubsan_encode_value (urhs, UBSAN_ENCODE_VALUE_GIMPLE);
      val = force_gimple_operand_gsi (&gsi2, val, true, NULL_TREE, true,
				      GSI_SAME_STMT);
      g = gimple_build_call (fn, 2, data, val);
    }
  gimple_set_location (g, loc);
  gsi_insert_before (&gsi2, g, GSI_SAME_STMT);
  ubsan_create_edge (g);
  *gsi = gsi_for_stmt (stmt);
}

/* Determine if we can propagate given LOCATION to ubsan_data descriptor to use
   new style handlers.  Libubsan uses heuristics to distinguish between old and
   new styles and relies on these properties for filename:

   a) Location's filename must not be NULL.
   b) Location's filename must not be equal to "".
   c) Location's filename must not be equal to "\1".
   d) First two bytes of filename must not contain '\xff' symbol.  */

static bool
ubsan_use_new_style_p (location_t loc)
{
  if (loc == UNKNOWN_LOCATION)
    return false;

  expanded_location xloc = expand_location (loc);
  if (xloc.file == NULL || startswith (xloc.file, "\1")
      || xloc.file[0] == '\0' || xloc.file[0] == '\xff'
      || xloc.file[1] == '\xff')
    return false;

  return true;
}

/* Build and return the boolean predicate that is true when converting the
   floating-point value EXPR to the integer TYPE would be out of range -- i.e.
   the truncated-toward-zero value is not representable in TYPE (core-language
   UB, [conv.fpint]).  Returns NULL_TREE if the conversion can never be out of
   range, or the floating mode is unsupported.  Shared by the UBSan float-cast
   instrumentation and the P3100 implicit-contract-assertion guard.  */

tree
ubsan_float_cast_overflow_predicate (tree type, tree expr)
{
  tree expr_type = TREE_TYPE (expr);
  tree t, tt, min, max;
  machine_mode mode = TYPE_MODE (expr_type);
  int prec = TYPE_PRECISION (type);
  bool uns_p = TYPE_UNSIGNED (type);

  /* Float to integer conversion first truncates toward zero, so
     even signed char c = 127.875f; is not problematic.
     Therefore, we should complain only if EXPR is unordered or smaller
     or equal than TYPE_MIN_VALUE - 1.0 or greater or equal than
     TYPE_MAX_VALUE + 1.0.  */
  if (REAL_MODE_FORMAT (mode)->b == 2)
    {
      /* For maximum, TYPE_MAX_VALUE might not be representable
	 in EXPR_TYPE, e.g. if TYPE is 64-bit long long and
	 EXPR_TYPE is IEEE single float, but TYPE_MAX_VALUE + 1.0 is
	 either representable or infinity.  */
      REAL_VALUE_TYPE maxval = dconst1;
      SET_REAL_EXP (&maxval, REAL_EXP (&maxval) + prec - !uns_p);
      real_convert (&maxval, mode, &maxval);
      max = build_real (expr_type, maxval);

      /* For unsigned, assume -1.0 is always representable.  */
      if (uns_p)
	min = build_minus_one_cst (expr_type);
      else
	{
	  /* TYPE_MIN_VALUE is generally representable (or -inf),
	     but TYPE_MIN_VALUE - 1.0 might not be.  */
	  REAL_VALUE_TYPE minval = dconstm1, minval2;
	  SET_REAL_EXP (&minval, REAL_EXP (&minval) + prec - 1);
	  real_convert (&minval, mode, &minval);
	  real_arithmetic (&minval2, MINUS_EXPR, &minval, &dconst1);
	  real_convert (&minval2, mode, &minval2);
	  if (real_compare (EQ_EXPR, &minval, &minval2)
	      && !real_isinf (&minval))
	    {
	      /* If TYPE_MIN_VALUE - 1.0 is not representable and
		 rounds to TYPE_MIN_VALUE, we need to subtract
		 more.  As REAL_MODE_FORMAT (mode)->p is the number
		 of base digits, we want to subtract a number that
		 will be 1 << (REAL_MODE_FORMAT (mode)->p - 1)
		 times smaller than minval.  */
	      minval2 = dconst1;
	      gcc_assert (prec > REAL_MODE_FORMAT (mode)->p);
	      SET_REAL_EXP (&minval2,
			    REAL_EXP (&minval2) + prec - 1
			    - REAL_MODE_FORMAT (mode)->p + 1);
	      real_arithmetic (&minval2, MINUS_EXPR, &minval, &minval2);
	      real_convert (&minval2, mode, &minval2);
	    }
	  min = build_real (expr_type, minval2);
	}
    }
  else if (REAL_MODE_FORMAT (mode)->b == 10)
    {
      /* For _Decimal128 up to 34 decimal digits, - sign,
	 dot, e, exponent.  */
      char buf[64];
      int p = REAL_MODE_FORMAT (mode)->p;
      REAL_VALUE_TYPE maxval, minval;

      /* Use mpfr_snprintf rounding to compute the smallest
	 representable decimal number greater or equal than
	 1 << (prec - !uns_p).  */
      auto_mpfr m (prec + 2);
      mpfr_set_ui_2exp (m, 1, prec - !uns_p, MPFR_RNDN);
      mpfr_snprintf (buf, sizeof buf, "%.*RUe", p - 1, (mpfr_srcptr) m);
      decimal_real_from_string (&maxval, buf);
      max = build_real (expr_type, maxval);

      /* For unsigned, assume -1.0 is always representable.  */
      if (uns_p)
	min = build_minus_one_cst (expr_type);
      else
	{
	  /* Use mpfr_snprintf rounding to compute the largest
	     representable decimal number less or equal than
	     (-1 << (prec - 1)) - 1.  */
	  mpfr_set_si_2exp (m, -1, prec - 1, MPFR_RNDN);
	  mpfr_sub_ui (m, m, 1, MPFR_RNDN);
	  mpfr_snprintf (buf, sizeof buf, "%.*RDe", p - 1, (mpfr_srcptr) m);
	  decimal_real_from_string (&minval, buf);
	  min = build_real (expr_type, minval);
	}
    }
  else
    return NULL_TREE;

  if (HONOR_NANS (mode))
    {
      t = fold_build2 (UNLE_EXPR, boolean_type_node, expr, min);
      tt = fold_build2 (UNGE_EXPR, boolean_type_node, expr, max);
    }
  else
    {
      t = fold_build2 (LE_EXPR, boolean_type_node, expr, min);
      tt = fold_build2 (GE_EXPR, boolean_type_node, expr, max);
    }
  t = fold_build2 (TRUTH_OR_EXPR, boolean_type_node, t, tt);
  if (integer_zerop (t))
    return NULL_TREE;
  return t;
}

/* Instrument float point-to-integer conversion.  TYPE is an integer type of
   destination, EXPR is floating-point expression.  */

tree
ubsan_instrument_float_cast (location_t loc, tree type, tree expr)
{
  tree expr_type = TREE_TYPE (expr);
  tree fn;
  tree t = ubsan_float_cast_overflow_predicate (type, expr);
  if (t == NULL_TREE)
    return NULL_TREE;
  if (loc == UNKNOWN_LOCATION)
    loc = input_location;

  if (flag_sanitize_trap & SANITIZE_FLOAT_CAST)
    fn = build_call_expr_loc (loc, builtin_decl_explicit (BUILT_IN_TRAP), 0);
  else
    {
      location_t *loc_ptr = NULL;
      unsigned num_locations = 0;
      /* Figure out if we can propagate location to ubsan_data and use new
         style handlers in libubsan.  */
      if (ubsan_use_new_style_p (loc))
	{
	  loc_ptr = &loc;
	  num_locations = 1;
	}
      /* Create the __ubsan_handle_float_cast_overflow fn call.  */
      tree data = ubsan_create_data ("__ubsan_float_cast_overflow_data",
				     num_locations, loc_ptr,
				     ubsan_type_descriptor (expr_type),
				     ubsan_type_descriptor (type), NULL_TREE,
				     NULL_TREE);
      enum built_in_function bcode
	= (flag_sanitize_recover & SANITIZE_FLOAT_CAST)
	  ? BUILT_IN_UBSAN_HANDLE_FLOAT_CAST_OVERFLOW
	  : BUILT_IN_UBSAN_HANDLE_FLOAT_CAST_OVERFLOW_ABORT;
      fn = builtin_decl_explicit (bcode);
      fn = build_call_expr_loc (loc, fn, 2,
				build_fold_addr_expr_loc (loc, data),
				ubsan_encode_value (expr));
    }

  return fold_build3 (COND_EXPR, void_type_node, t, fn, integer_zero_node);
}

/* Instrument values passed to function arguments with nonnull attribute.  */

static void
instrument_nonnull_arg (gimple_stmt_iterator *gsi)
{
  gimple *stmt = gsi_stmt (*gsi);
  location_t loc[2];
  /* infer_nonnull_range needs flag_delete_null_pointer_checks set,
     while for nonnull sanitization it is clear.  */
  int save_flag_delete_null_pointer_checks = flag_delete_null_pointer_checks;
  flag_delete_null_pointer_checks = 1;
  loc[0] = gimple_location (stmt);
  loc[1] = UNKNOWN_LOCATION;
  for (unsigned int i = 0; i < gimple_call_num_args (stmt); i++)
    {
      tree arg = gimple_call_arg (stmt, i);
      tree arg2, arg3;
      if (POINTER_TYPE_P (TREE_TYPE (arg))
	  && infer_nonnull_range_by_attribute (stmt, arg, &arg2, &arg3))
	{
	  gimple *g;
	  if (!is_gimple_val (arg))
	    {
	      g = gimple_build_assign (make_ssa_name (TREE_TYPE (arg)), arg);
	      gimple_set_location (g, loc[0]);
	      gsi_safe_insert_before (gsi, g);
	      arg = gimple_assign_lhs (g);
	    }
	  if (arg2 == arg3)
	    arg3 = NULL_TREE;
	  if (arg2 && !is_gimple_val (arg2))
	    {
	      g = gimple_build_assign (make_ssa_name (TREE_TYPE (arg2)), arg2);
	      gimple_set_location (g, loc[0]);
	      gsi_safe_insert_before (gsi, g);
	      arg2 = gimple_assign_lhs (g);
	    }
	  if (arg3 && !is_gimple_val (arg3))
	    {
	      g = gimple_build_assign (make_ssa_name (TREE_TYPE (arg3)), arg3);
	      gimple_set_location (g, loc[0]);
	      gsi_safe_insert_before (gsi, g);
	      arg3 = gimple_assign_lhs (g);
	    }

	  basic_block then_bb, fallthru_bb;
	  *gsi = create_cond_insert_point (gsi, true, false, true,
					   &then_bb, &fallthru_bb);
	  g = gimple_build_cond (EQ_EXPR, arg,
				 build_zero_cst (TREE_TYPE (arg)),
				 NULL_TREE, NULL_TREE);
	  gimple_set_location (g, loc[0]);
	  gsi_insert_after (gsi, g, GSI_NEW_STMT);

	  *gsi = gsi_after_labels (then_bb);
	  if (arg2)
	    {
	      *gsi = create_cond_insert_point (gsi, true, false, true,
					       &then_bb, &fallthru_bb);
	      g = gimple_build_cond (NE_EXPR, arg2,
				     build_zero_cst (TREE_TYPE (arg2)),
				     NULL_TREE, NULL_TREE);
	      gimple_set_location (g, loc[0]);
	      gsi_insert_after (gsi, g, GSI_NEW_STMT);

	      *gsi = gsi_after_labels (then_bb);
	    }
	  if (arg3)
	    {
	      *gsi = create_cond_insert_point (gsi, true, false, true,
					       &then_bb, &fallthru_bb);
	      g = gimple_build_cond (NE_EXPR, arg3,
				     build_zero_cst (TREE_TYPE (arg3)),
				     NULL_TREE, NULL_TREE);
	      gimple_set_location (g, loc[0]);
	      gsi_insert_after (gsi, g, GSI_NEW_STMT);

	      *gsi = gsi_after_labels (then_bb);
	    }
	  if (flag_sanitize_trap & SANITIZE_NONNULL_ATTRIBUTE)
	    g = gimple_build_call (builtin_decl_explicit (BUILT_IN_TRAP), 0);
	  else
	    {
	      tree data = ubsan_create_data ("__ubsan_nonnull_arg_data",
					     2, loc, NULL_TREE,
					     build_int_cst (integer_type_node,
							    i + 1),
					     NULL_TREE);
	      data = build_fold_addr_expr_loc (loc[0], data);
	      enum built_in_function bcode
		= (flag_sanitize_recover & SANITIZE_NONNULL_ATTRIBUTE)
		  ? BUILT_IN_UBSAN_HANDLE_NONNULL_ARG
		  : BUILT_IN_UBSAN_HANDLE_NONNULL_ARG_ABORT;
	      tree fn = builtin_decl_explicit (bcode);

	      g = gimple_build_call (fn, 1, data);
	    }
	  gimple_set_location (g, loc[0]);
	  gsi_safe_insert_before (gsi, g);
	  ubsan_create_edge (g);
	}
      *gsi = gsi_for_stmt (stmt);
    }
  flag_delete_null_pointer_checks = save_flag_delete_null_pointer_checks;
}

/* Instrument returns in functions with returns_nonnull attribute.  */

static void
instrument_nonnull_return (gimple_stmt_iterator *gsi)
{
  greturn *stmt = as_a <greturn *> (gsi_stmt (*gsi));
  location_t loc[2];
  tree arg = gimple_return_retval (stmt);
  /* infer_nonnull_range needs flag_delete_null_pointer_checks set,
     while for nonnull return sanitization it is clear.  */
  int save_flag_delete_null_pointer_checks = flag_delete_null_pointer_checks;
  flag_delete_null_pointer_checks = 1;
  loc[0] = gimple_location (stmt);
  loc[1] = UNKNOWN_LOCATION;
  if (arg
      && POINTER_TYPE_P (TREE_TYPE (arg))
      && is_gimple_val (arg)
      && infer_nonnull_range_by_attribute (stmt, arg))
    {
      basic_block then_bb, fallthru_bb;
      *gsi = create_cond_insert_point (gsi, true, false, true,
				       &then_bb, &fallthru_bb);
      gimple *g = gimple_build_cond (EQ_EXPR, arg,
				    build_zero_cst (TREE_TYPE (arg)),
				    NULL_TREE, NULL_TREE);
      gimple_set_location (g, loc[0]);
      gsi_insert_after (gsi, g, GSI_NEW_STMT);

      *gsi = gsi_after_labels (then_bb);
      if (flag_sanitize_trap & SANITIZE_RETURNS_NONNULL_ATTRIBUTE)
	g = gimple_build_call (builtin_decl_explicit (BUILT_IN_TRAP), 0);
      else
	{
	  tree data = ubsan_create_data ("__ubsan_nonnull_return_data",
					 1, &loc[1], NULL_TREE, NULL_TREE);
	  data = build_fold_addr_expr_loc (loc[0], data);
	  tree data2 = ubsan_create_data ("__ubsan_nonnull_return_data",
					  1, &loc[0], NULL_TREE, NULL_TREE);
	  data2 = build_fold_addr_expr_loc (loc[0], data2);
	  enum built_in_function bcode
	    = (flag_sanitize_recover & SANITIZE_RETURNS_NONNULL_ATTRIBUTE)
	      ? BUILT_IN_UBSAN_HANDLE_NONNULL_RETURN_V1
	      : BUILT_IN_UBSAN_HANDLE_NONNULL_RETURN_V1_ABORT;
	  tree fn = builtin_decl_explicit (bcode);

	  g = gimple_build_call (fn, 2, data, data2);
	}
      gimple_set_location (g, loc[0]);
      gsi_safe_insert_before (gsi, g);
      ubsan_create_edge (g);
      *gsi = gsi_for_stmt (stmt);
    }
  flag_delete_null_pointer_checks = save_flag_delete_null_pointer_checks;
}

/* Instrument memory references.  Here we check whether the pointer
   points to an out-of-bounds location.  */

static void
instrument_object_size (gimple_stmt_iterator *gsi, tree t, bool is_lhs)
{
  gimple *stmt = gsi_stmt (*gsi);
  location_t loc = gimple_location (stmt);
  tree type;
  tree index = NULL_TREE;
  HOST_WIDE_INT size_in_bytes;

  type = TREE_TYPE (t);
  if (VOID_TYPE_P (type))
    return;

  switch (TREE_CODE (t))
    {
    case COMPONENT_REF:
      if (TREE_CODE (t) == COMPONENT_REF
	  && DECL_BIT_FIELD_REPRESENTATIVE (TREE_OPERAND (t, 1)) != NULL_TREE)
	{
	  tree repr = DECL_BIT_FIELD_REPRESENTATIVE (TREE_OPERAND (t, 1));
	  t = build3 (COMPONENT_REF, TREE_TYPE (repr), TREE_OPERAND (t, 0),
		      repr, TREE_OPERAND (t, 2));
	}
      break;
    case ARRAY_REF:
      index = TREE_OPERAND (t, 1);
      break;
    case INDIRECT_REF:
    case MEM_REF:
    case VAR_DECL:
    case PARM_DECL:
    case RESULT_DECL:
      break;
    default:
      return;
    }

  size_in_bytes = int_size_in_bytes (type);
  if (size_in_bytes <= 0)
    return;

  poly_int64 bitsize, bitpos;
  tree offset;
  machine_mode mode;
  int volatilep = 0, reversep, unsignedp = 0;
  tree inner = get_inner_reference (t, &bitsize, &bitpos, &offset, &mode,
				    &unsignedp, &reversep, &volatilep);

  if (!multiple_p (bitpos, BITS_PER_UNIT)
      || maybe_ne (bitsize, size_in_bytes * BITS_PER_UNIT))
    return;

  bool decl_p = DECL_P (inner);
  tree base;
  if (decl_p)
    {
      if ((VAR_P (inner)
	   || TREE_CODE (inner) == PARM_DECL
	   || TREE_CODE (inner) == RESULT_DECL)
	  && DECL_REGISTER (inner))
	return;
      if (t == inner && !is_global_var (t))
	return;
      base = inner;
    }
  else if (TREE_CODE (inner) == MEM_REF)
    base = TREE_OPERAND (inner, 0);
  else
    return;
  tree ptr = build1 (ADDR_EXPR, build_pointer_type (TREE_TYPE (t)), t);

  while (TREE_CODE (base) == SSA_NAME)
    {
      gimple *def_stmt = SSA_NAME_DEF_STMT (base);
      if (gimple_assign_ssa_name_copy_p (def_stmt)
	  || (gimple_assign_cast_p (def_stmt)
	      && POINTER_TYPE_P (TREE_TYPE (gimple_assign_rhs1 (def_stmt))))
	  || (is_gimple_assign (def_stmt)
	      && gimple_assign_rhs_code (def_stmt) == POINTER_PLUS_EXPR))
	{
	  tree rhs1 = gimple_assign_rhs1 (def_stmt);
	  if (TREE_CODE (rhs1) == SSA_NAME
	      && SSA_NAME_OCCURS_IN_ABNORMAL_PHI (rhs1))
	    break;
	  else
	    base = rhs1;
	}
      else
	break;
    }

  if (!POINTER_TYPE_P (TREE_TYPE (base)) && !DECL_P (base))
    return;

  tree sizet;
  tree base_addr = base;
  gimple *bos_stmt = NULL;
  gimple_seq seq = NULL;
  if (decl_p)
    base_addr = build1 (ADDR_EXPR,
			build_pointer_type (TREE_TYPE (base)), base);
  if (compute_builtin_object_size (base_addr, OST_DYNAMIC, &sizet))
    ;
  else if (optimize)
    {
      if (LOCATION_LOCUS (loc) == UNKNOWN_LOCATION)
	loc = input_location;
      /* Generate __builtin_dynamic_object_size call.  */
      sizet = builtin_decl_explicit (BUILT_IN_DYNAMIC_OBJECT_SIZE);
      sizet = build_call_expr_loc (loc, sizet, 2, base_addr,
				   integer_zero_node);
      sizet = force_gimple_operand (sizet, &seq, false, NULL_TREE);
      /* If the call above didn't end up being an integer constant, go one
	 statement back and get the __builtin_object_size stmt.  Save it,
	 we might need it later.  */
      if (SSA_VAR_P (sizet))
	bos_stmt = gsi_stmt (gsi_last (seq));
    }
  else
    return;

  /* Generate UBSAN_OBJECT_SIZE (ptr, ptr+sizeof(*ptr)-base, objsize, ckind)
     call.  */
  /* ptr + sizeof (*ptr) - base */
  t = fold_build2 (MINUS_EXPR, sizetype,
		   fold_convert (pointer_sized_int_node, ptr),
		   fold_convert (pointer_sized_int_node, base_addr));
  t = fold_build2 (PLUS_EXPR, sizetype, t, TYPE_SIZE_UNIT (type));

  /* Perhaps we can omit the check.  */
  if (TREE_CODE (t) == INTEGER_CST
      && TREE_CODE (sizet) == INTEGER_CST
      && tree_int_cst_le (t, sizet))
    return;

  if (index != NULL_TREE
      && TREE_CODE (index) == SSA_NAME
      && TREE_CODE (sizet) == INTEGER_CST)
    {
      gimple *def = SSA_NAME_DEF_STMT (index);
      if (is_gimple_assign (def)
	  && gimple_assign_rhs_code (def) == BIT_AND_EXPR
	  && TREE_CODE (gimple_assign_rhs2 (def)) == INTEGER_CST)
	{
	  tree cst = gimple_assign_rhs2 (def);
	  tree sz = fold_build2 (EXACT_DIV_EXPR, sizetype, sizet,
				 TYPE_SIZE_UNIT (type));
	  if (tree_int_cst_sgn (cst) >= 0
	      && tree_int_cst_lt (cst, sz))
	    return;
	}
    }

  if (DECL_P (base)
      && decl_function_context (base) == current_function_decl
      && !TREE_ADDRESSABLE (base))
    mark_addressable (base);

  /* We have to emit the check.  */
  gimple_seq this_seq;
  t = force_gimple_operand (t, &this_seq, true, NULL_TREE);
  gimple_seq_add_seq_without_update (&seq, this_seq);
  ptr = force_gimple_operand (ptr, &this_seq, true, NULL_TREE);
  gimple_seq_add_seq_without_update (&seq, this_seq);
  gsi_safe_insert_seq_before (gsi, seq);

  if (bos_stmt
      && gimple_call_builtin_p (bos_stmt, BUILT_IN_DYNAMIC_OBJECT_SIZE))
    ubsan_create_edge (bos_stmt);

  tree ckind = build_int_cst (unsigned_char_type_node,
			      is_lhs ? UBSAN_STORE_OF : UBSAN_LOAD_OF);
  gimple *g = gimple_build_call_internal (IFN_UBSAN_OBJECT_SIZE, 4,
					 ptr, t, sizet, ckind);
  gimple_set_location (g, loc);
  gsi_safe_insert_before (gsi, g);
}

/* Instrument values passed to builtin functions.  */

static void
instrument_builtin (gimple_stmt_iterator *gsi)
{
  gimple *stmt = gsi_stmt (*gsi);
  location_t loc = gimple_location (stmt);
  tree arg;
  enum built_in_function fcode
    = DECL_FUNCTION_CODE (gimple_call_fndecl (stmt));
  int kind = 0;
  switch (fcode)
    {
    CASE_INT_FN (BUILT_IN_CLZ):
      kind = 1;
      gcc_fallthrough ();
    CASE_INT_FN (BUILT_IN_CTZ):
      arg = gimple_call_arg (stmt, 0);
      if (!integer_nonzerop (arg))
	{
	  gimple *g;
	  if (!is_gimple_val (arg))
	    {
	      g = gimple_build_assign (make_ssa_name (TREE_TYPE (arg)), arg);
	      gimple_set_location (g, loc);
	      gsi_insert_before (gsi, g, GSI_SAME_STMT);
	      arg = gimple_assign_lhs (g);
	    }

	  basic_block then_bb, fallthru_bb;
	  *gsi = create_cond_insert_point (gsi, true, false, true,
					   &then_bb, &fallthru_bb);
	  g = gimple_build_cond (EQ_EXPR, arg,
				 build_zero_cst (TREE_TYPE (arg)),
				 NULL_TREE, NULL_TREE);
	  gimple_set_location (g, loc);
	  gsi_insert_after (gsi, g, GSI_NEW_STMT);

	  *gsi = gsi_after_labels (then_bb);
	  if (flag_sanitize_trap & SANITIZE_BUILTIN)
	    g = gimple_build_call (builtin_decl_explicit (BUILT_IN_TRAP), 0);
	  else
	    {
	      tree t = build_int_cst (unsigned_char_type_node, kind);
	      tree data = ubsan_create_data ("__ubsan_builtin_data",
					     1, &loc, NULL_TREE, t, NULL_TREE);
	      data = build_fold_addr_expr_loc (loc, data);
	      enum built_in_function bcode
		= (flag_sanitize_recover & SANITIZE_BUILTIN)
		  ? BUILT_IN_UBSAN_HANDLE_INVALID_BUILTIN
		  : BUILT_IN_UBSAN_HANDLE_INVALID_BUILTIN_ABORT;
	      tree fn = builtin_decl_explicit (bcode);

	      g = gimple_build_call (fn, 1, data);
	    }
	  gimple_set_location (g, loc);
	  gsi_insert_before (gsi, g, GSI_SAME_STMT);
	  ubsan_create_edge (g);
	}
      *gsi = gsi_for_stmt (stmt);
      break;
    default:
      break;
    }
}

namespace {

const pass_data pass_data_ubsan =
{
  GIMPLE_PASS, /* type */
  "ubsan", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_TREE_UBSAN, /* tv_id */
  ( PROP_cfg | PROP_ssa ), /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  TODO_update_ssa, /* todo_flags_finish */
};

class pass_ubsan : public gimple_opt_pass
{
public:
  pass_ubsan (gcc::context *ctxt)
    : gimple_opt_pass (pass_data_ubsan, ctxt)
  {}

  /* opt_pass methods: */
  bool gate (function *) final override
    {
      /* P3100 implicit contract assertions reuse the null instrumentation
	 machinery even without any sanitizer enabled; run the pass so the
	 per-site null checks can be inserted (a no-op when nothing resolves
	 to a checked semantic).  */
      return flag_contracts_p3100
	     || sanitize_flags_p ((SANITIZE_NULL | SANITIZE_SI_OVERFLOW
				| SANITIZE_BOOL | SANITIZE_ENUM
				| SANITIZE_ALIGNMENT
				| SANITIZE_NONNULL_ATTRIBUTE
				| SANITIZE_RETURNS_NONNULL_ATTRIBUTE
				| SANITIZE_OBJECT_SIZE
				| SANITIZE_POINTER_OVERFLOW
				| SANITIZE_BUILTIN));
    }

  unsigned int execute (function *) final override;

}; // class pass_ubsan

unsigned int
pass_ubsan::execute (function *fun)
{
  basic_block bb;
  gimple_stmt_iterator gsi;
  unsigned int ret = 0;

  initialize_sanitizer_builtins ();

  FOR_EACH_BB_FN (bb, fun)
    {
      for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi);)
	{
	  gimple *stmt = gsi_stmt (gsi);
	  if (is_gimple_debug (stmt) || gimple_clobber_p (stmt))
	    {
	      gsi_next (&gsi);
	      continue;
	    }

	  if ((sanitize_flags_p (SANITIZE_SI_OVERFLOW, fun->decl)
	       || flag_contracts_p3100)
	      && is_gimple_assign (stmt))
	    {
	      instrument_si_overflow (&gsi);
	      /* The P3100 lowering can rewrite STMT (and split the block); keep
		 STMT and BB in step with the iterator for the checks below.  */
	      stmt = gsi_stmt (gsi);
	      bb = gimple_bb (stmt);
	    }

	  bool null_align_sanitize
	    = sanitize_flags_p (SANITIZE_NULL | SANITIZE_ALIGNMENT, fun->decl);
	  if (null_align_sanitize || flag_contracts_p3100)
	    {
	      /* Loads and stores through a pointer are genuine dereferences,
		 checkable both by the sanitizer and by a P3100
		 ub:expr.unary.dereference.nullptr implicit assertion.  */
	      if (gimple_store_p (stmt))
		instrument_null (gsi, gimple_get_lhs (stmt), true);
	      if (gimple_assign_single_p (stmt))
		instrument_null (gsi, gimple_assign_rhs1 (stmt), false);
	      /* Call arguments are a nonnull-attribute concern, not a
		 dereference; only the sanitizer instruments them.  */
	      if (null_align_sanitize && is_gimple_call (stmt))
		{
		  unsigned args_num = gimple_call_num_args (stmt);
		  for (unsigned i = 0; i < args_num; ++i)
		    {
		      tree arg = gimple_call_arg (stmt, i);
		      if (is_gimple_reg (arg) || is_gimple_min_invariant (arg))
			continue;
		      instrument_null (gsi, arg, false);
		    }
		}
	    }

	  if (sanitize_flags_p (SANITIZE_BOOL | SANITIZE_ENUM, fun->decl)
	      && gimple_assign_load_p (stmt))
	    {
	      instrument_bool_enum_load (&gsi);
	      bb = gimple_bb (stmt);
	    }
	  else if (flag_contracts_p3100
		   && gimple_assign_load_p (stmt)
		   && (TREE_CODE (TREE_TYPE (gimple_assign_rhs1 (stmt)))
			 == BOOLEAN_TYPE
		       || TREE_CODE (TREE_TYPE (gimple_assign_rhs1 (stmt)))
			 == ENUMERAL_TYPE))
	    {
	      /* P3100 implicit invalid-value-load assertion; resolve the site
		 once here (pre-inline) and lower it if it is a checking or
		 defining semantic.  */
	      int reaction
		= implicit_invalid_value_reaction (gimple_location (stmt));
	      if (reaction != IMPLICIT_UB_NONE)
		{
		  instrument_bool_enum_load_contract (&gsi, reaction);
		  stmt = gsi_stmt (gsi);
		  bb = gimple_bb (stmt);
		}
	    }

	  if (sanitize_flags_p (SANITIZE_NONNULL_ATTRIBUTE, fun->decl)
	      && is_gimple_call (stmt)
	      && !gimple_call_internal_p (stmt))
	    {
	      instrument_nonnull_arg (&gsi);
	      bb = gimple_bb (stmt);
	    }

	  if (sanitize_flags_p (SANITIZE_BUILTIN, fun->decl)
	      && gimple_call_builtin_p (stmt, BUILT_IN_NORMAL))
	    {
	      instrument_builtin (&gsi);
	      bb = gimple_bb (stmt);
	    }

	  if (sanitize_flags_p (SANITIZE_RETURNS_NONNULL_ATTRIBUTE, fun->decl)
	      && gimple_code (stmt) == GIMPLE_RETURN)
	    {
	      instrument_nonnull_return (&gsi);
	      bb = gimple_bb (stmt);
	    }

	  if (sanitize_flags_p (SANITIZE_OBJECT_SIZE, fun->decl))
	    {
	      if (gimple_store_p (stmt))
		instrument_object_size (&gsi, gimple_get_lhs (stmt), true);
	      if (gimple_assign_load_p (stmt))
		instrument_object_size (&gsi, gimple_assign_rhs1 (stmt),
					false);
	      if (is_gimple_call (stmt))
		{
		  unsigned args_num = gimple_call_num_args (stmt);
		  for (unsigned i = 0; i < args_num; ++i)
		    {
		      tree arg = gimple_call_arg (stmt, i);
		      if (is_gimple_reg (arg) || is_gimple_min_invariant (arg))
			continue;
		      instrument_object_size (&gsi, arg, false);
		    }
		}
	    }

	  if (sanitize_flags_p (SANITIZE_POINTER_OVERFLOW, fun->decl))
	    {
	      if (is_gimple_assign (stmt)
		  && gimple_assign_rhs_code (stmt) == POINTER_PLUS_EXPR)
		instrument_pointer_overflow (&gsi,
					     gimple_assign_rhs1 (stmt),
					     gimple_assign_rhs2 (stmt));
	      if (gimple_store_p (stmt))
		maybe_instrument_pointer_overflow (&gsi,
						   gimple_get_lhs (stmt));
	      if (gimple_assign_single_p (stmt))
		maybe_instrument_pointer_overflow (&gsi,
						   gimple_assign_rhs1 (stmt));
	      if (is_gimple_call (stmt))
		{
		  unsigned args_num = gimple_call_num_args (stmt);
		  for (unsigned i = 0; i < args_num; ++i)
		    {
		      tree arg = gimple_call_arg (stmt, i);
		      if (is_gimple_reg (arg))
			continue;
		      maybe_instrument_pointer_overflow (&gsi, arg);
		    }
		}
	    }

	  gsi_next (&gsi);
	}
      if (gimple_purge_dead_eh_edges (bb))
	ret = TODO_cleanup_cfg;
    }
  return ret;
}

} // anon namespace

gimple_opt_pass *
make_pass_ubsan (gcc::context *ctxt)
{
  return new pass_ubsan (ctxt);
}

#include "gt-ubsan.h"
