/* UndefinedBehaviorSanitizer, undefined behavior detector.
   Copyright (C) 2014-2026 Free Software Foundation, Inc.
   Contributed by Jakub Jelinek <jakub@redhat.com>

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
#include "cp-tree.h"
#include "ubsan.h"
#include "c-family/c-ubsan.h"
#include "stringpool.h"
#include "attribs.h"
#include "asan.h"
#include "opts.h"
#include "c-family/contracts-config.h"
#include "contracts.h"

/* Test if we should instrument vptr access.  */

static bool
cp_ubsan_instrument_vptr_p (tree type)
{
  if (!flag_rtti || (flag_sanitize_trap & SANITIZE_VPTR))
    return false;

  if (!sanitize_flags_p (SANITIZE_VPTR))
    return false;

  /* P3100: a vptr check resolved to "assume" is not instrumented.  The
     no_sanitize attribute c_apply_contract_sanitizer_assume adds before
     genericization comes too late for the casts instrumented while
     parsing.  */
  if (flag_contracts_p3100
      && resolved_sanitizer_semantic (SANITIZE_VPTR) == CES_ASSUME)
    return false;

  if (current_function_decl == NULL_TREE)
    return false;

  if (type)
    {
      type = TYPE_MAIN_VARIANT (type);
      if (!CLASS_TYPE_P (type) || !CLASSTYPE_VTABLES (type))
	return false;
    }

  return true;
}

static tree p3100_vptr_guard_pointer (location_t, tree);
static tree p3100_guard_vptr_check (location_t, tree, tree);
static tree p3100_vptr_checked_member_object (location_t, tree);
static bool p3100_checked_object_pointer_p (tree);

/* Helper function for
   cp_ubsan_maybe_instrument_{member_{call,access},downcast}.
   Instrument vptr access.  */

static tree
cp_ubsan_instrument_vptr (location_t loc, tree op, tree type, bool is_addr,
			  enum ubsan_null_ckind ckind)
{
  type = TYPE_MAIN_VARIANT (type);
  const char *mangled = mangle_type_string (type);
  hashval_t str_hash1 = htab_hash_string (mangled);
  hashval_t str_hash2 = iterative_hash (mangled, strlen (mangled), 0);
  tree str_hash = wide_int_to_tree (uint64_type_node,
				    wi::uhwi (((uint64_t) str_hash1 << 32)
					      | str_hash2, 64));
  if (!is_addr)
    op = build_fold_addr_expr_loc (loc, op);
  /* P3100: the vptr check of a member call or access, of a downcast of a
     glvalue or of its conversion to a virtual base skips a null object
     pointer (p3100_vptr_guard_pointer).  */
  tree guard = NULL_TREE;
  if (ckind == UBSAN_MEMBER_CALL
      || ckind == UBSAN_MEMBER_ACCESS
      || ckind == UBSAN_DOWNCAST_REFERENCE
      || ckind == UBSAN_CAST_TO_VBASE)
    guard = p3100_vptr_guard_pointer (loc, op);
  /* P3100: an address with no side effects is formed again where it is
     used, as without the check, so that what is read through it is
     checked for null on the pointer it is formed from (`p + 1' of `p[1]',
     DECISIONS.md E97 (c)).  */
  if (!guard || TREE_SIDE_EFFECTS (op))
    op = save_expr (op);
  tree vptr = build_vfield_ref (build_fold_indirect_ref_loc (loc, op), type);
  vptr = fold_convert_loc (loc, pointer_sized_int_node, vptr);
  vptr = fold_convert_loc (loc, uint64_type_node, vptr);
  if (ckind == UBSAN_DOWNCAST_POINTER)
    {
      tree cond = build2_loc (loc, NE_EXPR, boolean_type_node, op,
			      build_zero_cst (TREE_TYPE (op)));
      /* This is a compiler generated comparison, don't emit
	 e.g. -Wnonnull-compare warning for it.  */
      suppress_warning (cond, OPT_Wnonnull_compare);
      vptr = build3_loc (loc, COND_EXPR, uint64_type_node, cond,
			 vptr, build_int_cst (uint64_type_node, 0));
    }
  tree ti_decl = get_tinfo_decl (type);
  mark_used (ti_decl);
  tree ptype = build_pointer_type (type);
  tree call
    = build_call_expr_internal_loc (loc, IFN_UBSAN_VPTR,
				    void_type_node, 5, op, vptr, str_hash,
				    build_address (ti_decl),
				    build_int_cst (ptype, ckind));
  TREE_SIDE_EFFECTS (call) = 1;
  if (guard)
    {
      /* OP is evaluated first, as the call would evaluate it.  */
      call = build2 (COMPOUND_EXPR, void_type_node,
		     fold_convert (void_type_node, op),
		     p3100_guard_vptr_check (loc, guard, call));
      return build2 (COMPOUND_EXPR, TREE_TYPE (op), call, op);
    }
  return fold_build2 (COMPOUND_EXPR, TREE_TYPE (op), call, op);
}

/* Helper function for
   cp_ubsan_maybe_instrument_{member_{call,access},downcast}.
   Instrument vptr access if it should be instrumented, otherwise return
   NULL_TREE.  */

static tree
cp_ubsan_maybe_instrument_vptr (location_t loc, tree op, tree type,
				bool is_addr, enum ubsan_null_ckind ckind)
{
  if (!cp_ubsan_instrument_vptr_p (type))
    return NULL_TREE;
  return cp_ubsan_instrument_vptr (loc, op, type, is_addr, ckind);
}

/* Instrument a member call (but not constructor call) if needed.  */

void
cp_ubsan_maybe_instrument_member_call (tree stmt)
{
  if (call_expr_nargs (stmt) == 0)
    return;
  tree op, *opp;

  tree fn = CALL_EXPR_FN (stmt);
  if (fn && TREE_CODE (fn) == OBJ_TYPE_REF)
    {
      /* Virtual function call: Sanitize the use of the object pointer in the
	 OBJ_TYPE_REF, since the vtable reference will SEGV otherwise (95221).
	 OBJ_TYPE_REF_EXPR is ptr->vptr[N] and OBJ_TYPE_REF_OBJECT is ptr.  But
	 we can't be sure of finding OBJ_TYPE_REF_OBJECT in OBJ_TYPE_REF_EXPR
	 if the latter has been optimized, so we use a COMPOUND_EXPR below.  */
      opp = &OBJ_TYPE_REF_EXPR (fn);
      op = OBJ_TYPE_REF_OBJECT (fn);
      /* P3100: after the null check of the object's pointer made before
	 the vtable lookup or with the object operand
	 (cp_ubsan_maybe_check_virtual_call_object), not again.  */
      while (TREE_CODE (*opp) == COMPOUND_EXPR
	     && p3100_checked_object_pointer_p (*opp))
	opp = &TREE_OPERAND (*opp, 1);
      while (TREE_CODE (op) == COMPOUND_EXPR
	     && p3100_checked_object_pointer_p (op))
	op = TREE_OPERAND (op, 1);
    }
  else
    {
      /* Non-virtual call: Sanitize the 'this' argument.  */
      opp = &CALL_EXPR_ARG (stmt, 0);
      if (*opp == error_mark_node
	  || !INDIRECT_TYPE_P (TREE_TYPE (*opp)))
	return;
      while (TREE_CODE (*opp) == COMPOUND_EXPR)
	opp = &TREE_OPERAND (*opp, 1);
      op = *opp;
    }
  op = cp_ubsan_maybe_instrument_vptr (EXPR_LOCATION (stmt), op,
				       TREE_TYPE (TREE_TYPE (op)),
				       true, UBSAN_MEMBER_CALL);
  if (!op)
    /* No change.  */;
  else if (fn && TREE_CODE (fn) == OBJ_TYPE_REF)
    *opp = cp_build_compound_expr (op, *opp, tf_none);
  else
    *opp = op;
}

/* Data passed to cp_ubsan_check_member_access_r.  */

struct cp_ubsan_check_member_access_data
{
  hash_set<tree> *pset;
  bool is_addr;
};

static tree cp_ubsan_check_member_access_r (tree *, int *, void *);

/* Instrument a member access.  */

static bool
cp_ubsan_maybe_instrument_member_access
     (tree stmt, cp_ubsan_check_member_access_data *ucmd)
{
  if (DECL_ARTIFICIAL (TREE_OPERAND (stmt, 1)))
    return false;

  tree base = TREE_OPERAND (stmt, 0);
  if (!cp_ubsan_instrument_vptr_p (TREE_TYPE (base)))
    return false;

  cp_walk_tree (&base, cp_ubsan_check_member_access_r, ucmd, ucmd->pset);

  /* P3100: the vptr check skips a null object pointer
     (p3100_vptr_guard_pointer), and the member is accessed as it would be
     without it, not through the address the check tests: an address formed
     from a null pointer is checked for null as an indirection of its own.  */
  if (tree access = p3100_vptr_checked_member_object (EXPR_LOCATION (stmt),
						     base))
    {
      TREE_OPERAND (stmt, 0) = access;
      return true;
    }

  base = cp_ubsan_instrument_vptr (EXPR_LOCATION (stmt), base,
				   TREE_TYPE (base), false,
				   UBSAN_MEMBER_ACCESS);
  TREE_OPERAND (stmt, 0)
    = build_fold_indirect_ref_loc (EXPR_LOCATION (stmt), base);
  return true;
}

/* Attempt to instrument member accesses inside of the function.
   cp_ubsan_maybe_instrument_member_access should be called on COMPONENT_REFs
   in the GENERIC IL, but only when the field is actually accessed, not
   merely when it's address is taken.  Therefore we track in is_addr field
   whether in the current context we are processing address taken
   handled components or not.  E.g. for &x->y[w->z] we want to call
   cp_ubsan_maybe_instrument_member_access on *w.z COMPONENT_REF, but
   not on *x.y.  */

static tree
cp_ubsan_check_member_access_r (tree *stmt_p, int *walk_subtrees, void *data)
{
  tree stmt = *stmt_p, t;
  cp_ubsan_check_member_access_data *ucmd
    = (cp_ubsan_check_member_access_data *) data;
  switch (TREE_CODE (stmt))
    {
    case ADDR_EXPR:
      t = TREE_OPERAND (stmt, 0);
      while ((TREE_CODE (t) == MEM_REF || INDIRECT_REF_P (t))
	     && TREE_CODE (TREE_OPERAND (t, 0)) == ADDR_EXPR)
	t = TREE_OPERAND (TREE_OPERAND (t, 0), 0);
      if (handled_component_p (t))
	{
	  *walk_subtrees = 0;
	  ucmd->is_addr = true;
	  cp_walk_tree (&t, cp_ubsan_check_member_access_r,
			data, ucmd->pset);
	  ucmd->is_addr = false;
	}
      break;
    case MEM_REF:
    case INDIRECT_REF:
      t = TREE_OPERAND (stmt, 0);
      if (TREE_CODE (t) == ADDR_EXPR)
	{
	  *walk_subtrees = 0;
	  t = TREE_OPERAND (t, 0);
	  cp_walk_tree (&t, cp_ubsan_check_member_access_r, data, ucmd->pset);
	}
      break;
    case COMPONENT_REF:
      if (!ucmd->is_addr && cp_ubsan_maybe_instrument_member_access (stmt, ucmd))
	{
	  *walk_subtrees = 0;
	  break;
	}
      /* FALLTHRU */
    default:
      if (ucmd->is_addr && handled_component_p (stmt))
	{
	  int i, len = TREE_OPERAND_LENGTH (stmt);
	  *walk_subtrees = 0;
	  if (!handled_component_p (TREE_OPERAND (stmt, 0)))
	    ucmd->is_addr = false;
	  for (i = 0; i < len; i++)
	    {
	      cp_walk_tree (&TREE_OPERAND (stmt, i),
			    cp_ubsan_check_member_access_r, data, ucmd->pset);
	      ucmd->is_addr = false;
	    }
	  ucmd->is_addr = true;
	}
      break;
    }
  return NULL_TREE;
}

/* Instrument all member accesses inside GENERIC *T_P.  */

void
cp_ubsan_instrument_member_accesses (tree *t_p)
{
  if (cp_ubsan_instrument_vptr_p (NULL_TREE))
    {
      hash_set<tree> pset;
      cp_ubsan_check_member_access_data ucmd;
      ucmd.pset = &pset;
      ucmd.is_addr = false;
      cp_walk_tree (t_p, cp_ubsan_check_member_access_r, &ucmd, &pset);
    }
}

/* P3100: the null check of the object pointer of OP->*PM, if PTR (the
   operand of an indirection) is the member's address formed from it: the
   IFN_UBSAN_NULL call ubsan_maybe_instrument_ptrmem_object built, with
   UBSAN_MEMBER_ACCESS, reached through the conversions, saved expressions,
   base and member offsets and guards build_m_component_ref wraps around
   it.  NULL_TREE if PTR is not such an address.  */

static tree
cp_ubsan_ptrmem_object_check (tree ptr)
{
  while (true)
    {
      STRIP_NOPS (ptr);
      switch (TREE_CODE (ptr))
	{
	case SAVE_EXPR:
	case POINTER_PLUS_EXPR:
	  ptr = TREE_OPERAND (ptr, 0);
	  break;
	case ADDR_EXPR:
	  /* The conversion to a base class: &(*ptr).base.  */
	  ptr = TREE_OPERAND (ptr, 0);
	  while (handled_component_p (ptr))
	    ptr = TREE_OPERAND (ptr, 0);
	  if (!INDIRECT_REF_P (ptr) && TREE_CODE (ptr) != MEM_REF)
	    return NULL_TREE;
	  ptr = TREE_OPERAND (ptr, 0);
	  break;
	case COND_EXPR:
	  if (tree call = cp_ubsan_ptrmem_object_check (TREE_OPERAND (ptr, 1)))
	    return call;
	  ptr = TREE_OPERAND (ptr, 2);
	  break;
	case COMPOUND_EXPR:
	  {
	    tree call = TREE_OPERAND (ptr, 0);
	    if (TREE_CODE (call) == CALL_EXPR
		&& CALL_EXPR_FN (call) == NULL_TREE
		&& CALL_EXPR_IFN (call) == IFN_UBSAN_NULL
		&& (tree_to_uhwi (CALL_EXPR_ARG (call, UBSAN_NULL_CKIND))
		    == UBSAN_MEMBER_ACCESS))
	      return call;
	    ptr = TREE_OPERAND (ptr, 1);
	    break;
	  }
	default:
	  return NULL_TREE;
	}
      if (ptr == NULL_TREE)
	return NULL_TREE;
    }
}

/* P3100: true if PTR, the object pointer of OP->*PM, is the `this' of the
   member function, or the `this' a lambda captured, through conversions
   and saved expressions.  */

static bool
cp_ubsan_this_pointer_p (tree ptr)
{
  while (true)
    {
      STRIP_NOPS (ptr);
      if (TREE_CODE (ptr) != SAVE_EXPR)
	break;
      ptr = TREE_OPERAND (ptr, 0);
    }
  if (is_this_parameter (ptr))
    return true;
  /* The `this' a lambda captured, `__closure->__this'.  */
  if (TREE_CODE (ptr) == COMPONENT_REF)
    {
      tree field = TREE_OPERAND (ptr, 1);
      return (TREE_CODE (field) == FIELD_DECL
	      && DECL_NAME (field) != NULL_TREE
	      && id_equal (DECL_NAME (field), "__this")
	      && POINTER_TYPE_P (TREE_TYPE (field)));
    }
  return false;
}

/* P3100: if ADDR, stripped of conversions and saves, is the address
   `p + c' of the element at a constant subscript of the pointer p
   (ubsan_p3100_constant_subscript) guarded against wrapping around the
   address space (build_implicit_pointer_wrap_check), `force, (wrapped ?
   <reaction / p> : p + c)', check p for null where nothing wraps, under
   the .access leaf if ACCESS, else .indirection: `force, (wrapped ?
   <reaction / p> : (<check p>, p + c))'.  The ubsan pass checks the
   address the indirection goes through, which is p + c, not null, where
   nothing wraps, and p where the arithmetic wrapped, reported then after
   the wrap -- as Clang does.  An unguarded `p + c' is checked on p by the
   ubsan pass (cp_gimplify_expr).  If p is a guarded `q + c2' itself, the
   chain `q + c2 + c', q is checked (ubsan_p3100_subscript_pointer).  */

static void
cp_ubsan_check_guarded_subscript (tree addr, bool access)
{
  while (true)
    {
      STRIP_NOPS (addr);
      if (TREE_CODE (addr) != SAVE_EXPR)
	break;
      addr = TREE_OPERAND (addr, 0);
    }
  if (TREE_CODE (addr) != COMPOUND_EXPR)
    return;
  /* The guard may have been folded (cp_fold_function), its flags lost,
     and a conversion of it folded through the comma.  */
  tree guard = TREE_OPERAND (addr, 1);
  STRIP_NOPS (guard);
  if (TREE_CODE (guard) != COND_EXPR)
    return;
  tree sum = TREE_OPERAND (guard, 2);
  STRIP_NOPS (sum);
  if (TREE_CODE (sum) != POINTER_PLUS_EXPR
      || !ubsan_p3100_constant_subscript (sum, 0))
    return;
  if (tree check
      = ubsan_build_p3100_indirection_check (EXPR_LOCATION (guard),
					     ubsan_p3100_subscript_pointer
					       (sum), access))
    TREE_OPERAND (guard, 2)
      = build2 (COMPOUND_EXPR, TREE_TYPE (TREE_OPERAND (guard, 2)), check,
		TREE_OPERAND (guard, 2));
}

/* Data passed to cp_ubsan_ptrmem_access_r.  */

struct cp_ubsan_ptrmem_access_data
{
  tree fndecl;
  hash_set<tree> *pset;
  hash_set<tree> *retargeted;
  /* The object of the clobber last walked, which is not accessed.  */
  tree clobbered;
};

/* P3100: walk *TP for indirections that access the object, and retarget
   the null check of a pointer-to-data-member access's object pointer that
   such an indirection reads or writes through to the .access leaf of
   ub:expr.unary.dereference.nullptr.  An indirection whose address is taken
   (&(p->*pm), &(p->*pa)[i]) accesses nothing, and its check stays the
   .indirection leaf; so does one folded away by the address being taken or
   a reference bound to it, or discarded.  A read or write through `this',
   `this->*pm' or `(*this).*pm' in the member function called or a lambda
   capturing it, is not checked at all (DECISIONS.md E100 (a)): the null
   dereference is reported where the member function is called.  Also
   check the pointer of an element at a guarded constant subscript that
   is accessed or has the address of a subobject taken
   (cp_ubsan_check_guarded_subscript).  */

static tree
cp_ubsan_ptrmem_access_r (tree *tp, int *walk_subtrees, void *data_)
{
  cp_ubsan_ptrmem_access_data *data = (cp_ubsan_ptrmem_access_data *) data_;
  tree t = *tp;
  switch (TREE_CODE (t))
    {
    case ADDR_EXPR:
      {
	/* Walk the address's indices and its base's pointer, but not the
	   base indirection itself.  */
	*walk_subtrees = 0;
	tree *p = &TREE_OPERAND (t, 0);
	bool subobject = false;
	while (handled_component_p (*p))
	  {
	    for (int i = 1; i < TREE_OPERAND_LENGTH (*p); ++i)
	      if (TREE_OPERAND (*p, i) && !DECL_P (TREE_OPERAND (*p, i)))
		cp_walk_tree (&TREE_OPERAND (*p, i), cp_ubsan_ptrmem_access_r,
			      data, data->pset);
	    p = &TREE_OPERAND (*p, 0);
	    subobject = true;
	  }
	/* The address of a subobject of the element at a guarded constant
	   subscript, `&sp[1].a'.  */
	if (subobject && INDIRECT_REF_P (*p))
	  cp_ubsan_check_guarded_subscript (TREE_OPERAND (*p, 0), false);
	if (INDIRECT_REF_P (*p) || TREE_CODE (*p) == MEM_REF)
	  p = &TREE_OPERAND (*p, 0);
	cp_walk_tree (p, cp_ubsan_ptrmem_access_r, data, data->pset);
	break;
      }
    case INDIRECT_REF:
    case MEM_REF:
      /* An access to the element at a guarded constant subscript.  */
      if (INDIRECT_REF_P (t) && t != data->clobbered)
	cp_ubsan_check_guarded_subscript (TREE_OPERAND (t, 0), true);
      if (tree call = cp_ubsan_ptrmem_object_check (TREE_OPERAND (t, 0)))
	if (!data->retargeted->add (call))
	  {
	    const char *group = "ub:expr.unary.dereference.nullptr.access";
	    location_t loc = EXPR_LOCATION (call);
	    int reaction
	      = (cp_ubsan_this_pointer_p (CALL_EXPR_ARG (call, UBSAN_NULL_PTR))
		 ? IMPLICIT_UB_NONE
		 : resolve_implicit_ub_reaction (data->fndecl, loc, group));
	    tree entry, data_addr;
	    implicit_ub_handler_operands (data->fndecl, loc, group, reaction,
					  &entry, &data_addr);
	    CALL_EXPR_ARG (call, UBSAN_NULL_REACTION)
	      = build_int_cst (unsigned_type_node, reaction);
	    CALL_EXPR_ARG (call, UBSAN_NULL_ENTRY) = entry;
	    CALL_EXPR_ARG (call, UBSAN_NULL_DATA) = data_addr;
	  }
      break;
    case MODIFY_EXPR:
    case INIT_EXPR:
      if (TREE_CLOBBER_P (TREE_OPERAND (t, 1)))
	data->clobbered = TREE_OPERAND (t, 0);
      break;
    default:
      if (TYPE_P (t))
	*walk_subtrees = 0;
      break;
    }
  return NULL_TREE;
}

/* P3100: the null check of the object pointer of OP->*PM is built as the
   .indirection leaf of ub:expr.unary.dereference.nullptr, since whether the
   member is read or written is not known where the expression is built.
   Once FNDECL's body is complete, retarget each one whose member is
   accessed to the .access leaf, as a check of p->m is.  */

void
cp_ubsan_retarget_ptrmem_object_checks (tree fndecl)
{
  if (!flag_contracts_p3100)
    return;
  hash_set<tree> pset, retargeted;
  cp_ubsan_ptrmem_access_data data = { fndecl, &pset, &retargeted,
				       NULL_TREE };
  cp_walk_tree (&DECL_SAVED_TREE (fndecl), cp_ubsan_ptrmem_access_r, &data,
		&pset);
}

/* Instrument downcast.  */

tree
cp_ubsan_maybe_instrument_downcast (location_t loc, tree type,
				    tree intype, tree op)
{
  if (!INDIRECT_TYPE_P (type)
      || !INDIRECT_TYPE_P (intype)
      || !INDIRECT_TYPE_P (TREE_TYPE (op))
      || !CLASS_TYPE_P (TREE_TYPE (TREE_TYPE (op)))
      || !is_properly_derived_from (TREE_TYPE (type), TREE_TYPE (intype)))
    return NULL_TREE;

  return cp_ubsan_maybe_instrument_vptr (loc, op, TREE_TYPE (type), true,
					 TYPE_PTR_P (type)
					 ? UBSAN_DOWNCAST_POINTER
					 : UBSAN_DOWNCAST_REFERENCE);
}

/* Instrument cast to virtual base.  */

tree
cp_ubsan_maybe_instrument_cast_to_vbase (location_t loc, tree type, tree op)
{
  return cp_ubsan_maybe_instrument_vptr (loc, op, type, true,
					 UBSAN_CAST_TO_VBASE);
}

/* Called from initialize_vtbl_ptrs via dfs_walk.  BINFO is the base
   which we want to initialize the vtable pointer for, DATA is
   TREE_LIST whose TREE_VALUE is the this ptr expression.  */

static tree
cp_ubsan_dfs_initialize_vtbl_ptrs (tree binfo, void *data)
{
  if (!TYPE_CONTAINS_VPTR_P (BINFO_TYPE (binfo)))
    return dfs_skip_bases;

  if (!BINFO_PRIMARY_P (binfo))
    {
      tree base_ptr = TREE_VALUE ((tree) data);

      base_ptr = build_base_path (PLUS_EXPR, base_ptr, binfo, /*nonnull=*/1,
				  tf_warning_or_error);

      /* Compute the location of the vtpr.  */
      tree vtbl_ptr
	= build_vfield_ref (cp_build_fold_indirect_ref (base_ptr),
			    TREE_TYPE (binfo));
      gcc_assert (vtbl_ptr != error_mark_node);

      /* Assign NULL to the vptr.  */
      tree vtbl = build_zero_cst (TREE_TYPE (vtbl_ptr));
      tree stmt = cp_build_modify_expr (input_location, vtbl_ptr, NOP_EXPR,
					vtbl, tf_warning_or_error);
      if (vptr_via_virtual_p (binfo))
	/* If this vptr comes from a virtual base of the complete object, only
	   clear it if we're in charge of virtual bases.  */
	stmt = build_if_in_charge (stmt);
      finish_expr_stmt (stmt);
    }

  return NULL_TREE;
}

/* Initialize all the vtable pointers in the object pointed to by
   ADDR to NULL, so that we catch invalid calls to methods before
   mem-initializers are completed.  */

void
cp_ubsan_maybe_initialize_vtbl_ptrs (tree addr)
{
  if (!cp_ubsan_instrument_vptr_p (NULL_TREE))
    return;

  tree type = TREE_TYPE (TREE_TYPE (addr));
  tree list = build_tree_list (type, addr);
  /* We cannot rely on the vtable being set up.  We have to indirect via the
     vtt_parm.  */
  int save_in_base_initializer = in_base_initializer;
  in_base_initializer = 1;

  /* Walk through the hierarchy, initializing the vptr in each base
     class to NULL.  */
  dfs_walk_once (TYPE_BINFO (type), cp_ubsan_dfs_initialize_vtbl_ptrs,
		 NULL, list);

  in_base_initializer = save_in_base_initializer;
}

/* P3100: *PTR_P is the operand of an indirection `*p' through a pointer
   whose result is not accessed: the operand of `&', a discarded
   expression, the object expression of a static member.  If the
   .indirection leaf of {expr.unary.dereference.nullptr} applies at LOC (the
   .access leaf if ACCESS), replace *PTR_P with a single-evaluation form and
   return the void check that it is not null, which evaluates it; otherwise
   return NULL_TREE.  A load or store through the pointer is checked by the
   ubsan pass instead, and `this' is not checked; nor is a pointer converted
   from a reference, which indirects through nothing (the object was reached
   where the reference was bound, and checked there).  The address `p + c'
   of the element at a constant subscript, `&sp[1]', `E e2 = ep[1]', is
   checked on p (DECISIONS.md E97 (c), ubsan_p3100_constant_subscript).  */

tree
cp_ubsan_maybe_check_unaccessed_indirection (location_t loc, tree *ptr_p,
					     bool access)
{
  tree ptr = *ptr_p;
  if (!flag_contracts_p3100
      || processing_template_decl
      || cp_unevaluated_operand
      || current_function_decl == NULL_TREE
      || ptr == error_mark_node
      || !POINTER_TYPE_P (TREE_TYPE (ptr)))
    return NULL_TREE;
  tree sub = ubsan_p3100_constant_subscript (ptr, 0);
  if (sub)
    ptr = ubsan_p3100_subscript_pointer (sub);
  tree stripped = tree_strip_nop_conversions (ptr);
  /* The p a guard against wrapping saved (ubsan_p3100_unguarded_sum).  */
  if (sub && TREE_CODE (stripped) == SAVE_EXPR)
    stripped = tree_strip_nop_conversions (TREE_OPERAND (stripped, 0));
  if (TREE_CODE (stripped) == ADDR_EXPR
      || is_this_parameter (stripped)
      || TYPE_REF_P (TREE_TYPE (stripped)))
    return NULL_TREE;
  tree saved = save_expr (ptr);
  tree check = ubsan_build_p3100_indirection_check (loc, saved, access);
  if (check)
    *ptr_p = (sub
	      ? fold_convert (TREE_TYPE (*ptr_p),
			      ubsan_p3100_rebuild_subscript (sub, saved,
							     *ptr_p))
	      : saved);
  return check;
}

/* walk_tree callback for p3100_virtual_base_object: the read of a virtual
   base's offset from the vtable, which build_base_path marks
   TREE_CONSTANT.  */

static tree
p3100_vbase_offset_r (tree *tp, int *walk_subtrees, void *)
{
  if (INDIRECT_REF_P (*tp) && TREE_CONSTANT (*tp))
    return *tp;
  if (TYPE_P (*tp))
    *walk_subtrees = 0;
  return NULL_TREE;
}

/* P3100: if PTR (stripped of conversions) is the address of a virtual base
   class subobject that build_base_path computed from the address of an
   object -- that address plus an offset read through the object's vtable
   -- return the object's address; otherwise NULL_TREE.  */

static tree
p3100_virtual_base_object (tree ptr)
{
  if (TREE_CODE (ptr) != POINTER_PLUS_EXPR
      || !walk_tree_without_duplicates (&TREE_OPERAND (ptr, 1),
					p3100_vbase_offset_r, NULL))
    return NULL_TREE;
  return TREE_OPERAND (ptr, 0);
}

/* P3100: true if CALL calls std::move, std::forward, std::forward_like or
   std::as_const, which return their argument, cast.  */

static bool
p3100_cast_call_p (tree call)
{
  if (TREE_CODE (call) != CALL_EXPR || call_expr_nargs (call) != 1)
    return false;
  tree callee = get_callee_fndecl (call);
  return (callee
	  && decl_in_std_namespace_p (callee)
	  && DECL_NAME (callee) != NULL_TREE
	  && (id_equal (DECL_NAME (callee), "move")
	      || id_equal (DECL_NAME (callee), "forward")
	      || id_equal (DECL_NAME (callee), "forward_like")
	      || id_equal (DECL_NAME (callee), "as_const")));
}

/* P3100: if the glvalue E is a subobject of an object reached by an
   indirection -- a chain of non-static data member accesses (base class
   subobjects with a field among them) and subscripts of array members over
   an INDIRECT_REF, `p->m', `p->a[1].m', `(*p).<base>.m' -- return that
   INDIRECT_REF, else NULL_TREE.  Add to *SIDE, in their order of
   evaluation, the subscripts with side effects, whose evaluation a caller
   dropping E keeps.  */

static tree
p3100_subobject_indirection (tree e, tree *side)
{
  e = tree_strip_any_location_wrapper (e);
  if (TREE_CODE (e) != COMPONENT_REF && TREE_CODE (e) != ARRAY_REF)
    return NULL_TREE;
  tree s = NULL_TREE;
  while (true)
    {
      if (TREE_CODE (e) == COMPONENT_REF)
	{
	  if (TREE_CODE (TREE_OPERAND (e, 1)) != FIELD_DECL)
	    return NULL_TREE;
	}
      else if (TREE_CODE (e) == ARRAY_REF)
	{
	  tree idx = TREE_OPERAND (e, 1);
	  if (TREE_SIDE_EFFECTS (idx))
	    {
	      idx = build1 (CONVERT_EXPR, void_type_node, idx);
	      s = s ? build2 (COMPOUND_EXPR, void_type_node, idx, s) : idx;
	    }
	}
      else
	break;
      e = tree_strip_any_location_wrapper (TREE_OPERAND (e, 0));
    }
  if (!INDIRECT_REF_P (e))
    return NULL_TREE;
  if (s)
    *side = *side ? build2 (COMPOUND_EXPR, void_type_node, s, *side) : s;
  return e;
}

/* P3100: true if the class BASE is a virtual base of the class DERIVED,
   or a base of one.  */

static bool
p3100_virtual_base_of_p (tree base, tree derived)
{
  if (!CLASS_TYPE_P (base)
      || !CLASS_TYPE_P (derived)
      || !COMPLETE_TYPE_P (derived))
    return false;
  base_kind kind;
  lookup_base (TYPE_MAIN_VARIANT (derived), TYPE_MAIN_VARIANT (base), ba_any,
	       &kind, tf_none);
  return kind == bk_via_virtual;
}

/* P3100: true if converting OBJ, a pointer to a class, to a pointer to
   the class BASE is a conversion to a virtual base.  */

static bool
p3100_virtual_base_conversion_p (tree base, tree obj)
{
  return p3100_virtual_base_of_p (base, TREE_TYPE (TREE_TYPE
					       (tree_strip_nop_conversions
						(obj))));
}

/* P3100: PTR is the address of an object whose glvalue is evaluated and
   not accessed.  Return the pointer that evaluating it indirects through:
   through member accesses, array member subscripts, base class
   conversions (`&p->m', `&p->a[1]', `&(*p).<base>', an empty base at
   `p + offset', ubsan_p3100_base_offset_object) and the calls of std::move
   and the like returning their argument, p; with *ACCESS set if reaching
   the object converts to a virtual base (whose offset is read through p,
   unless the complete type is known), and *SIDE extended
   with the side effects of the subscripts (p3100_subobject_indirection).
   NULL_TREE if it indirects through a glvalue that is a reference already
   -- a reference variable, member or parameter, a call returning a
   reference -- which indirects through nothing (the object was reached
   where the reference was bound, and checked there).  Otherwise PTR
   stripped of conversions, which may be the address of an object.  */

static tree
p3100_object_pointer (tree ptr, bool *access, tree *side)
{
  while (true)
    {
      tree op = tree_strip_nop_conversions (ptr);
      /* A virtual base of an object whose complete type is known is at a
	 constant offset (0 too), and its conversion is still the .access
	 leaf's (DECISIONS.md E94 (b)).  */
      tree base = TREE_TYPE (TREE_TYPE (ptr));
      if (op != ptr && p3100_virtual_base_conversion_p (base, op))
	*access = true;
      if (tree obj = ubsan_p3100_base_offset_object (op))
	{
	  if (p3100_virtual_base_conversion_p (base, obj))
	    *access = true;
	  ptr = obj;
	}
      else if (tree vobj = p3100_virtual_base_object (op))
	{
	  ptr = vobj;
	  *access = true;
	}
      else if (TREE_CODE (op) == ADDR_EXPR)
	{
	  tree ind = p3100_subobject_indirection (TREE_OPERAND (op, 0), side);
	  if (ind == NULL_TREE)
	    return ptr;
	  ptr = TREE_OPERAND (ind, 0);
	  if (REFERENCE_REF_P (ind))
	    {
	      tree ref = tree_strip_nop_conversions (ptr);
	      if (p3100_cast_call_p (ref))
		ptr = CALL_EXPR_ARG (ref, 0);
	      else if (!TYPE_PTR_P (TREE_TYPE (ref)))
		return NULL_TREE;
	    }
	}
      else
	return op;
    }
}

/* P3100: true if evaluating the glvalue E is an indirection through a
   pointer whose result it does not access, its address being the value
   of the pointer: `*p', a cast of `*p' to a reference (a reference
   conversion of a pointer that is not the address of an object or of a
   base class subobject, or that address formed by an offset: `static_cast<T
   &>', `reinterpret_cast<T &>', a C-style cast), or a conditional or comma
   expression yielding one.  */

bool
p3100_indirection_glvalue_p (tree e)
{
  e = tree_strip_any_location_wrapper (e);
  switch (TREE_CODE (e))
    {
    case INDIRECT_REF:
      if (!REFERENCE_REF_P (e))
	return true;
      e = tree_strip_nop_conversions (TREE_OPERAND (e, 0));
      return (TYPE_PTR_P (TREE_TYPE (e))
	      && TREE_CODE (e) != ADDR_EXPR
	      && !p3100_virtual_base_object (e)
	      && !ubsan_p3100_base_offset_object (e));
    case COND_EXPR:
      return ((TREE_OPERAND (e, 1)
	       && p3100_indirection_glvalue_p (TREE_OPERAND (e, 1)))
	      || p3100_indirection_glvalue_p (TREE_OPERAND (e, 2)));
    case COMPOUND_EXPR:
      return p3100_indirection_glvalue_p (TREE_OPERAND (e, 1));
    default:
      return false;
    }
}

/* P3100: EXP is the address of the glvalue ARG, formed by `&' or
   __builtin_addressof.  If ARG is an indirection through a pointer
   (p3100_indirection_glvalue_p), EXP is the pointer's value: return it
   checked not to be null under the .indirection leaf, if that applies at
   LOC, else EXP.  If ARG is an empty base class subobject of `*p' at a
   nonzero offset, check p (ubsan_p3100_check_base_offset_object).  If
   ARG is a virtual base class subobject of `*p', or a subobject of one,
   check p under the .access leaf (cp_ubsan_maybe_check_vbase_conversion).  */

tree
cp_ubsan_maybe_check_address_of (location_t loc, tree arg, tree exp)
{
  if (!flag_contracts_p3100 || exp == error_mark_node)
    return exp;
  /* The address of a virtual base class subobject of `*p', or of a member
     of one, `&static_cast<E &> (p->ve)', `&p->vb.b': check p under the
     .access leaf (DECISIONS.md E94 (b)).  */
  if (cp_ubsan_maybe_check_vbase_conversion (loc, &exp))
    return exp;
  /* The address of an empty base class subobject of `*p' at a nonzero
     offset, `&static_cast<E &> (*p)', is formed from p: check p.  */
  tree a = tree_strip_any_location_wrapper (arg);
  if (INDIRECT_REF_P (a) && REFERENCE_REF_P (a)
      && !processing_template_decl
      && !cp_unevaluated_operand)
    if (tree check = ubsan_p3100_check_base_offset_object (loc, &exp))
      return build2 (COMPOUND_EXPR, TREE_TYPE (exp), check, exp);
  if (!p3100_indirection_glvalue_p (arg))
    return exp;
  if (tree check = cp_ubsan_maybe_check_unaccessed_indirection (loc, &exp))
    exp = build2 (COMPOUND_EXPR, TREE_TYPE (exp), check, exp);
  return exp;
}

/* P3100: E is a glvalue whose result is not accessed: a discarded
   expression, the object expression of a static member, an enumerator or a
   pseudo-destructor, the object of a trivial destructor call, the source
   of an elided copy of an empty class.  If evaluating it is an indirection
   through a pointer (p3100_indirection_glvalue_p), the conversion of `*p'
   to a base class, or a member or array member element of `*p' (`p->m',
   p3100_subobject_indirection), and the leaf of
   {expr.unary.dereference.nullptr} that applies at LOC does, return the
   void check that p is not null, which evaluates every side effect of E;
   otherwise NULL_TREE.  The conversion to a virtual base reads the base's
   offset through p, so it is checked under the .access leaf; every other
   under .indirection (p3100_object_pointer).  A reference is not checked.  */

tree
cp_ubsan_maybe_check_unaccessed_glvalue (location_t loc, tree e)
{
  if (!flag_contracts_p3100
      || processing_template_decl
      || cp_unevaluated_operand
      || current_function_decl == NULL_TREE
      || e == NULL_TREE
      || e == error_mark_node)
    return NULL_TREE;
  e = tree_strip_any_location_wrapper (e);
  tree side = NULL_TREE;
  if (TREE_CODE (e) == COMPONENT_REF || TREE_CODE (e) == ARRAY_REF)
    {
      e = p3100_subobject_indirection (e, &side);
      if (e == NULL_TREE)
	return NULL_TREE;
    }
  tree ptr;
  if (INDIRECT_REF_P (e))
    {
      ptr = TREE_OPERAND (e, 0);
      /* A reference is checked only where it converts a pointer: a cast of
	 `*p' to a reference, `*p' converted to a base class -- also the
	 element at a constant subscript guarded against wrapping, whose
	 conversion to a reference fold distributes over the guard.  */
      if (REFERENCE_REF_P (e)
	  && !TYPE_PTR_P (TREE_TYPE (tree_strip_nop_conversions (ptr)))
	  && !ubsan_p3100_guarded_subscript_p (ptr))
	return NULL_TREE;
    }
  else if ((TREE_CODE (e) == COND_EXPR || TREE_CODE (e) == COMPOUND_EXPR)
	   && p3100_indirection_glvalue_p (e))
    ptr = cp_build_addr_expr (e, tf_none);
  else
    return NULL_TREE;
  bool access = false;
  ptr = p3100_object_pointer (ptr, &access, &side);
  if (ptr == NULL_TREE)
    return NULL_TREE;
  bool guarded = ubsan_p3100_guarded_subscript_p (ptr);
  tree check = cp_ubsan_maybe_check_unaccessed_indirection (loc, &ptr, access);
  /* The check evaluates p; the guard against wrapping, the arithmetic
     after it.  */
  if (check && guarded)
    check = build2 (COMPOUND_EXPR, void_type_node, check,
		    fold_convert (void_type_node, ptr));
  if (check && side)
    check = build2 (COMPOUND_EXPR, void_type_node, check, side);
  return check;
}

/* P3100: true if the glvalue E is a virtual base class subobject of
   `*p', whose address build_base_path computes by reading the base's
   offset through p (p3100_virtual_base_object).  */

bool
cp_ubsan_virtual_base_glvalue_p (tree e)
{
  e = tree_strip_any_location_wrapper (e);
  return (INDIRECT_REF_P (e)
	  && p3100_virtual_base_object (tree_strip_nop_conversions
					(TREE_OPERAND (e, 0))));
}

/* P3100: if PTR is the address of a virtual base class subobject of an
   object, or of a subobject of one -- a conversion to a virtual base at a
   constant offset, `(E *) &p->ve', a base field of a virtual base among
   member accesses, `&p->vb.<B>.b', or at an offset read through the
   object's vtable, `&p->va[1] + *(...)' (p3100_virtual_base_object) --
   return the address of that object; otherwise NULL_TREE.  */

static tree
p3100_vbase_conversion_object (tree ptr)
{
  tree t = ptr;
  while (CONVERT_EXPR_P (t) || TREE_CODE (t) == NON_LVALUE_EXPR)
    {
      tree op = TREE_OPERAND (t, 0);
      if (TYPE_PTR_P (TREE_TYPE (t))
	  && INDIRECT_TYPE_P (TREE_TYPE (op))
	  && p3100_virtual_base_conversion_p (TREE_TYPE (TREE_TYPE (t)), op))
	return op;
      t = op;
    }
  if (tree obj = p3100_virtual_base_object (t))
    return obj;
  if (TREE_CODE (t) != ADDR_EXPR)
    return NULL_TREE;
  for (tree e = TREE_OPERAND (t, 0); handled_component_p (e);
       e = TREE_OPERAND (e, 0))
    if (TREE_CODE (e) == COMPONENT_REF
	&& TREE_CODE (TREE_OPERAND (e, 1)) == FIELD_DECL
	&& DECL_FIELD_IS_BASE (TREE_OPERAND (e, 1))
	&& p3100_virtual_base_of_p (TREE_TYPE (TREE_OPERAND (e, 1)),
				    TREE_TYPE (TREE_OPERAND (e, 0))))
      return t;
  return NULL_TREE;
}

/* P3100: return T with every occurrence of FROM replaced by TO, copying
   only the nodes on the way to one (each once, MAP recording the copies,
   so that a shared node such as a SAVE_EXPR stays shared).  */

static tree
p3100_replace_operand (tree t, tree from, tree to, hash_map<tree, tree> *map)
{
  if (t == from
      || (!TREE_SIDE_EFFECTS (from)
	  && TREE_TYPE (t) == TREE_TYPE (from)
	  && operand_equal_p (t, from, 0)))
    return to;
  if (!EXPR_P (t))
    return t;
  if (tree *copy = map->get (t))
    return *copy;
  tree r = t;
  for (int i = 0; i < TREE_OPERAND_LENGTH (t); ++i)
    {
      tree op = TREE_OPERAND (t, i);
      if (op == NULL_TREE)
	continue;
      tree nop = p3100_replace_operand (op, from, to, map);
      if (nop != op)
	{
	  if (r == t)
	    r = copy_node (t);
	  TREE_OPERAND (r, i) = nop;
	}
    }
  map->put (t, r);
  return r;
}

/* P3100: true if PTR, stripped of conversions and saved expressions, is
   an object pointer the front end checked, the check first: the object of
   OP->*PM (ubsan_maybe_instrument_ptrmem_object), a rewritten conversion to
   a virtual base (cp_ubsan_maybe_check_vbase_conversion).  */

static bool
p3100_checked_object_pointer_p (tree ptr)
{
  while (true)
    {
      ptr = tree_strip_nop_conversions (ptr);
      if (TREE_CODE (ptr) == SAVE_EXPR)
	ptr = TREE_OPERAND (ptr, 0);
      else
	break;
    }
  if (TREE_CODE (ptr) != COMPOUND_EXPR)
    return false;
  tree call = TREE_OPERAND (ptr, 0);
  return (TREE_CODE (call) == CALL_EXPR
	  && CALL_EXPR_FN (call) == NULL_TREE
	  && CALL_EXPR_IFN (call) == IFN_UBSAN_NULL
	  && (tree_to_uhwi (CALL_EXPR_ARG (call, UBSAN_NULL_CKIND))
	      == UBSAN_MEMBER_ACCESS));
}

/* P3100: the null check of SAVED, a pointer evaluated once, under the
   leaf GROUP of {expr.unary.dereference.nullptr}, with no reaction if
   GROUP does not apply at LOC.  It is the kind the object pointer of
   OP->*PM gets (UBSAN_MEMBER_ACCESS), so the ubsan pass checks no address
   formed from SAVED (ubsan_ptrmem_derived_p), whatever its reaction.  */

static tree
p3100_object_pointer_check (location_t loc, tree saved, const char *group)
{
  int reaction = resolve_implicit_ub_reaction (current_function_decl, loc,
					       group);
  tree entry, data_addr;
  implicit_ub_handler_operands (current_function_decl, loc, group, reaction,
				&entry, &data_addr);
  tree call
    = build_call_expr_internal_loc (loc, IFN_UBSAN_NULL, void_type_node,
				    UBSAN_NULL_NUM_OPS, saved,
				    build_int_cst (TREE_TYPE (saved),
						   UBSAN_MEMBER_ACCESS),
				    build_int_cst (pointer_sized_int_node, 0),
				    build_int_cst (unsigned_type_node,
						   reaction),
				    entry, data_addr,
				    build_int_cst (unsigned_type_node,
						   IMPLICIT_UB_NONE),
				    null_pointer_node, null_pointer_node);
  TREE_SIDE_EFFECTS (call) = 1;
  return call;
}

/* P3100: whether a null check of the pointer PTR, the pointer an object
   is reached through, is wanted at LOC: a leaf of
   {expr.unary.dereference.nullptr} applies, and PTR is not a reference,
   `this' (or a lambda's captured `this'), the address of an object, a
   constant or checked already.  */

static bool
p3100_object_pointer_check_p (location_t loc, tree ptr)
{
  if (ptr == NULL_TREE
      || !TYPE_PTR_P (TREE_TYPE (ptr))
      || TREE_CODE (ptr) == ADDR_EXPR
      || CONSTANT_CLASS_P (ptr)
      || is_this_parameter (ptr)
      || p3100_checked_object_pointer_p (ptr))
    return false;
  return ((resolve_implicit_ub_reaction
	   (current_function_decl, loc,
	    "ub:expr.unary.dereference.nullptr.access")
	   != IMPLICIT_UB_NONE)
	  || (resolve_implicit_ub_reaction
	      (current_function_decl, loc,
	       "ub:expr.unary.dereference.nullptr.indirection")
	      != IMPLICIT_UB_NONE));
}

/* P3100: the pointer the object at the address ADDR is reached through
   (p3100_object_pointer, through pointer offsets too), which the vptr check
   of a member call or access of the object, of a downcast of it to a
   reference or of its conversion to a virtual base tests for null first, if
   a leaf of {expr.unary.dereference.nullptr} applies at LOC: the null
   dereference is the null checks' to report, P3100's or -fsanitize=null's,
   each once, and the vptr check, which reads the vtable pointer through the
   pointer, is not made on a null one (as Clang's is not).  Made first, the
   vptr check would report the null dereference again, under the leaf of a
   read, or read through the null pointer before the null check of a
   subobject's pointer.  NULL_TREE if
   the object is not reached through such a pointer -- the address of an
   object, a reference, `this' -- or the pointer cannot be evaluated again;
   the vptr check is then made as it is without P3100.  GCC-856.  */

static tree
p3100_vptr_guard_pointer (location_t loc, tree addr)
{
  if (!flag_contracts_p3100
      || processing_template_decl
      || current_function_decl == NULL_TREE
      || ((resolve_implicit_ub_reaction
	   (current_function_decl, loc,
	    "ub:expr.unary.dereference.nullptr.access")
	   == IMPLICIT_UB_NONE)
	  && (resolve_implicit_ub_reaction
	      (current_function_decl, loc,
	       "ub:expr.unary.dereference.nullptr.indirection")
	      == IMPLICIT_UB_NONE)))
    return NULL_TREE;
  bool access = false;
  tree side = NULL_TREE;
  tree ptr = addr;
  while (true)
    {
      ptr = p3100_object_pointer (ptr, &access, &side);
      if (ptr == NULL_TREE)
	break;
      if (TREE_CODE (ptr) == POINTER_PLUS_EXPR)
	ptr = TREE_OPERAND (ptr, 0);
      /* The object pointer of a call through a pointer to member function,
	 checked before its adjustment
	 (cp_ubsan_maybe_instrument_ptrmemfunc_call), and evaluated before
	 the guard is.  */
      else if (TREE_CODE (ptr) == COMPOUND_EXPR)
	ptr = TREE_OPERAND (ptr, 1);
      /* An address the -fsanitize=null check of a member call saved,
	 evaluated before the guard is: the pointer it is formed from, `p'
	 of `&p->b', `p + 16' (GCC-865).  */
      else if (TREE_CODE (ptr) == SAVE_EXPR
	       && !SAVE_EXPR_RESOLVED_P (ptr)
	       && !TREE_SIDE_EFFECTS (TREE_OPERAND (ptr, 0))
	       && (TREE_CODE (tree_strip_nop_conversions (TREE_OPERAND (ptr,
									0)))
		   == ADDR_EXPR
		   || (TREE_CODE (tree_strip_nop_conversions
				  (TREE_OPERAND (ptr, 0)))
		       == POINTER_PLUS_EXPR)))
	ptr = TREE_OPERAND (ptr, 0);
      else
	{
	  if (TREE_CODE (ptr) == TARGET_EXPR)
	    ptr = TARGET_EXPR_SLOT (ptr);
	  break;
	}
    }
  if (ptr == NULL_TREE
      || side != NULL_TREE
      || !TYPE_PTR_P (TREE_TYPE (ptr))
      || TREE_CODE (ptr) == ADDR_EXPR
      || CONSTANT_CLASS_P (ptr)
      || cp_ubsan_this_pointer_p (ptr)
      || (TREE_SIDE_EFFECTS (ptr) && TREE_CODE (ptr) != SAVE_EXPR))
    return NULL_TREE;
  return ptr;
}

/* P3100: CALL, the vptr check of a member call or access, made only if
   GUARD (p3100_vptr_guard_pointer) is not null.  */

static tree
p3100_guard_vptr_check (location_t loc, tree guard, tree call)
{
  tree cond = build2_loc (loc, NE_EXPR, boolean_type_node, guard,
			  build_zero_cst (TREE_TYPE (guard)));
  suppress_warning (cond, OPT_Wnonnull_compare);
  return build3_loc (loc, COND_EXPR, void_type_node, cond, call, void_node);
}

/* P3100: data for p3100_count_pointer_r.  */

struct p3100_pointer_count
{
  tree ptr;
  int count;
};

/* P3100: walk_tree callback counting the occurrences of the pointer of
   *DATA, a p3100_pointer_count.  */

static tree
p3100_count_pointer_r (tree *tp, int *walk_subtrees, void *data)
{
  p3100_pointer_count *d = (p3100_pointer_count *) data;
  if (*tp == d->ptr
      || (!TREE_SIDE_EFFECTS (d->ptr)
	  && TREE_TYPE (*tp) == TREE_TYPE (d->ptr)
	  && operand_equal_p (*tp, d->ptr, 0)))
    {
      ++d->count;
      *walk_subtrees = 0;
    }
  return NULL_TREE;
}

/* P3100: BASE being the object of a member access the vptr sanitizer
   instruments, `p->m', `p->b.m' for a base or member b, reached through a
   pointer p the vptr check tests for null first (p3100_vptr_guard_pointer):
   return BASE with p replaced by the vptr check, guarded, and p, so that the
   member is accessed through p as without the check; NULL_TREE if the check
   is made as it is without P3100 -- also if BASE has side effects other
   than p's, or names p more than once, since the check evaluates it again.
   GCC-856.  */

static tree
p3100_vptr_checked_member_object (location_t loc, tree base)
{
  tree addr = build_fold_addr_expr_loc (loc, base);
  tree ptr = p3100_vptr_guard_pointer (loc, addr);
  if (ptr == NULL_TREE)
    return NULL_TREE;
  tree t = base;
  while (true)
    {
      if (TREE_THIS_VOLATILE (t))
	return NULL_TREE;
      if (TREE_CODE (t) == COMPONENT_REF)
	t = TREE_OPERAND (t, 0);
      else if (TREE_CODE (t) == ARRAY_REF)
	{
	  if (TREE_SIDE_EFFECTS (TREE_OPERAND (t, 1)))
	    return NULL_TREE;
	  t = TREE_OPERAND (t, 0);
	}
      else
	break;
    }
  if (!INDIRECT_REF_P (t))
    return NULL_TREE;
  p3100_pointer_count count = { ptr, 0 };
  walk_tree_without_duplicates (&base, p3100_count_pointer_r, &count);
  tree inner = TREE_OPERAND (t, 0);
  if (count.count != 1
      || (TREE_SIDE_EFFECTS (inner)
	  && tree_strip_nop_conversions (inner) != ptr))
    return NULL_TREE;
  tree r = cp_ubsan_instrument_vptr (loc, base, TREE_TYPE (base), false,
				     UBSAN_MEMBER_ACCESS);
  /* (OP, CHECK), OP: drop OP's evaluation first, so that the address is
     formed only where p is not null.  */
  gcc_checking_assert (TREE_CODE (r) == COMPOUND_EXPR
		       && TREE_CODE (TREE_OPERAND (r, 0)) == COMPOUND_EXPR);
  tree check = TREE_OPERAND (TREE_OPERAND (r, 0), 1);
  hash_map<tree, tree> map;
  return p3100_replace_operand (base, ptr,
				build2 (COMPOUND_EXPR, TREE_TYPE (ptr), check,
					ptr), &map);
}

/* P3100: *OP_P is a pointer whose object OBJ (*OP_P itself, or an object
   it is a subobject of) is reached through a pointer p
   (p3100_object_pointer), and evaluating *OP_P reads through p: check p,
   saved, under the .access leaf of {expr.unary.dereference.nullptr} (no
   reaction if only .indirection applies), rewrite *OP_P to form the
   address from the saved p, and return true; see
   cp_ubsan_maybe_check_vbase_conversion.  If SUBSCRIPT, the element at a
   constant subscript, `p[1]', is checked on p (DECISIONS.md E97 (c)).  A
   p that is a reference, `this', the address of an object, a constant or
   checked already is not checked.  */

static bool
p3100_check_object_pointer_access (location_t loc, tree *op_p, tree obj,
				   bool subscript)
{
  tree op = *op_p;
  bool access = false;
  tree side = NULL_TREE;
  tree ptr = p3100_object_pointer (obj, &access, &side);
  if (subscript && ptr != NULL_TREE)
    if (tree sub = ubsan_p3100_constant_subscript (ptr, 0))
      ptr = ubsan_p3100_subscript_pointer (sub);
  if (!p3100_object_pointer_check_p (loc, ptr))
    return false;
  tree saved = build1_loc (loc, SAVE_EXPR, TREE_TYPE (ptr), ptr);
  TREE_SIDE_EFFECTS (saved) = 1;
  hash_map<tree, tree> map;
  tree addr = p3100_replace_operand (op, ptr, saved, &map);
  tree call
    = p3100_object_pointer_check (loc, saved,
				  "ub:expr.unary.dereference.nullptr.access");
  *op_p = build2 (COMPOUND_EXPR, TREE_TYPE (op), call, addr);
  return true;
}

/* P3100: *OP_P is the pointer a reference is bound to or a member function
   is called on, or the value of `&'; TYPE, if not NULL_TREE, the class the
   reference binding converts it to.  If it is the address of a virtual base
   class subobject, or of a subobject of one, of an object reached through a
   pointer p (p3100_vbase_conversion_object, p3100_object_pointer) --
   `E &r = p->ve', `p->vb.f ()', `&p->vb.b', `E &r = p->va[1]', `E &r =
   p[1]' -- the conversion to the virtual base is the .access leaf of
   {expr.unary.dereference.nullptr}, whether or not its offset is read
   (DECISIONS.md E94 (b)).  If a leaf of that group applies at LOC, rewrite
   *OP_P to check p, saved, under the .access leaf (no reaction if only
   .indirection applies) and to form the address from the saved p, and
   return true.  The check is the kind the object pointer of OP->*PM gets
   (UBSAN_MEMBER_ACCESS), so the ubsan pass checks no address formed from
   p after it (ubsan_ptrmem_derived_p) -- neither the member's address,
   .indirection, nor the read of a vtable pointer -- and
   ubsan_maybe_instrument_reference_or_call does not check *OP_P
   (ubsan_p3100_checked_subobject_p).  A p that is a reference, `this',
   the address of an object, a constant or checked already is not
   checked.  */

bool
cp_ubsan_maybe_check_vbase_conversion (location_t loc, tree *op_p,
				       tree type)
{
  if (!flag_contracts_p3100
      || processing_template_decl
      || cp_unevaluated_operand
      || current_function_decl == NULL_TREE)
    return false;
  tree op = *op_p;
  if (op == NULL_TREE
      || op == error_mark_node
      || !POINTER_TYPE_P (TREE_TYPE (op)))
    return false;
  tree obj = NULL_TREE;
  if (type && p3100_virtual_base_conversion_p (type, op))
    obj = op;
  else
    obj = p3100_vbase_conversion_object (op);
  /* A conversion saved for the vtable lookup of a virtual call, which it
     has side effects for (the vptr check of the conversion,
     cp_ubsan_maybe_instrument_cast_to_vbase): check p where it is first
     evaluated.  GCC-856.  */
  if (obj == NULL_TREE
      && TREE_CODE (op) == SAVE_EXPR
      && !SAVE_EXPR_RESOLVED_P (op)
      && (obj = p3100_vbase_conversion_object (TREE_OPERAND (op, 0))))
    op_p = &TREE_OPERAND (op, 0);
  if (obj == NULL_TREE)
    return false;
  return p3100_check_object_pointer_access (loc, op_p, obj, false);
}

/* P3100: *OP_P is the address of the object a dynamic_cast to a reference
   type reads the dynamic type of through its vtable pointer: the operand
   `*p', `p[1]', `p->m' bound to the reference the cast converts it to.
   Reading the vtable pointer through the pointer p the object is reached
   through is the .access leaf of {expr.unary.dereference.nullptr}, as a
   virtual call's is (DECISIONS.md E102 (a)): if a leaf of that group
   applies at LOC, rewrite *OP_P to check p, saved, under the .access leaf
   and to form the address from the saved p, and return true; the
   reference binding's own check is then not made
   (ubsan_p3100_checked_subobject_p).  The element at a constant subscript,
   `p[1]', is checked on p (DECISIONS.md E97 (c)).  An object reached
   through a reference is not checked again.  */

bool
cp_ubsan_maybe_check_dynamic_type_read (location_t loc, tree *op_p)
{
  if (!flag_contracts_p3100
      || processing_template_decl
      || cp_unevaluated_operand
      || current_function_decl == NULL_TREE)
    return false;
  tree op = *op_p;
  if (op == NULL_TREE
      || op == error_mark_node
      || !TYPE_PTR_P (TREE_TYPE (op)))
    return false;
  return p3100_check_object_pointer_access (loc, op_p, op, true);
}

/* P3100: STMT being a call of a virtual member function, `p->m.f ()',
   `p->a[1].f ()', `p->g ()' for a g of a base at a nonzero offset, whose
   object pointer is the address of a subobject of the object a pointer p
   points to: the call reads the subobject's vtable pointer through p,
   which is the .access leaf of {expr.unary.dereference.nullptr}
   (DECISIONS.md E102 (a)), but the address is computed first and the ubsan
   pass checks it on p under .indirection.  If a leaf of that group applies,
   check p, saved, under .access (no reaction if only .indirection applies)
   before the vtable pointer is read, form the subobject's addresses from
   the saved p, and return true.  The check is the kind the object pointer
   of OP->*PM gets (UBSAN_MEMBER_ACCESS), so the ubsan pass checks no
   address formed from p again (ubsan_ptrmem_derived_p), nor is the
   member call's own check of the object pointer made with a reaction.  A
   call on `*p' itself, whose object pointer is p, is checked by the read
   of the vtable pointer.  GCC-845.  So is a call of a virtual function by
   its unqualified name that the front end resolved statically
   (CALL_EXPR_P3100_VIRTUAL_P), `p->m.f ()' for a member m (whose dynamic
   type is known), `p->a->f ()', `p->g ()' for a `final' g, which reads no
   vtable pointer: its leaf follows the source (DECISIONS.md E110 (a)); p
   is checked before the call's argument is evaluated.  */

bool
cp_ubsan_maybe_check_virtual_call_object (tree stmt)
{
  if (!flag_contracts_p3100
      || processing_template_decl
      || current_function_decl == NULL_TREE
      || call_expr_nargs (stmt) == 0)
    return false;
  tree fn = CALL_EXPR_FN (stmt);
  bool direct = CALL_EXPR_P3100_VIRTUAL_P (stmt);
  if (fn == NULL_TREE || (TREE_CODE (fn) != OBJ_TYPE_REF && !direct))
    return false;
  tree *argp = &CALL_EXPR_ARG (stmt, 0);
  tree arg = *argp;
  if (arg == error_mark_node)
    return false;
  /* A virtual call on a virtual base of `*p', `p->f ()' for an f of the
     virtual base: the conversion's check of p
     (cp_ubsan_maybe_check_vbase_conversion), made in the argument, is made
     before the object operand and the vtable lookup, which are evaluated
     first and read the virtual base's offset through p too, and their
     addresses are formed from the saved p.  GCC-859.  */
  tree checked = tree_strip_nop_conversions (arg);
  if (TREE_CODE (fn) == OBJ_TYPE_REF
      && TREE_CODE (checked) == COMPOUND_EXPR
      && p3100_checked_object_pointer_p (checked))
    {
      tree call = TREE_OPERAND (checked, 0);
      tree saved = CALL_EXPR_ARG (call, 0);
      if (TREE_CODE (saved) != SAVE_EXPR)
	return false;
      tree ptr = TREE_OPERAND (saved, 0);
      hash_map<tree, tree> map;
      fn = copy_node (fn);
      tree obj = p3100_replace_operand (OBJ_TYPE_REF_OBJECT (fn), ptr, saved,
					&map);
      OBJ_TYPE_REF_OBJECT (fn) = build2 (COMPOUND_EXPR, TREE_TYPE (obj), call,
					 obj);
      OBJ_TYPE_REF_EXPR (fn)
	= p3100_replace_operand (OBJ_TYPE_REF_EXPR (fn), ptr, saved, &map);
      CALL_EXPR_FN (stmt) = fn;
      *argp = fold_convert (TREE_TYPE (arg), TREE_OPERAND (checked, 1));
      return true;
    }
  /* A virtual call on the derived object of a downcast of `*p' to a
     reference, or on a base class subobject of it, whose vptr check
     -fsanitize=vptr wrapped around the downcast, saved as the object
     argument: the vtable pointer is read through `p - offset', so check p
     (ubsan_p3100_vptr_checked_downcast).  */
  tree ptr = NULL_TREE;
  if (TREE_CODE (fn) == OBJ_TYPE_REF)
    {
      tree obj = arg;
      STRIP_NOPS (obj);
      if (TREE_CODE (obj) == SAVE_EXPR)
	obj = TREE_OPERAND (obj, 0);
      STRIP_NOPS (obj);
      /* A base class subobject of it, `&(<downcast>)->X'.  */
      if (TREE_CODE (obj) == ADDR_EXPR)
	{
	  tree base = TREE_OPERAND (obj, 0);
	  while (TREE_CODE (base) == COMPONENT_REF
		 && DECL_FIELD_IS_BASE (TREE_OPERAND (base, 1)))
	    base = TREE_OPERAND (base, 0);
	  if (base != TREE_OPERAND (obj, 0) && INDIRECT_REF_P (base))
	    obj = TREE_OPERAND (base, 0);
	}
      if (tree dsum = ubsan_p3100_vptr_checked_downcast (obj))
	ptr = TREE_OPERAND (dsum, 0);
    }
  if (ptr == NULL_TREE)
    {
      if (TREE_CODE (checked) != ADDR_EXPR)
	return false;
      bool access = false;
      tree side = NULL_TREE;
      ptr = p3100_object_pointer (arg, &access, &side);
      if (ptr != NULL_TREE)
	if (tree sub = ubsan_p3100_constant_subscript (ptr, 0))
	  ptr = ubsan_p3100_subscript_pointer (sub);
    }
  location_t loc = EXPR_LOCATION (stmt);
  if (!p3100_object_pointer_check_p (loc, ptr)
      || cp_ubsan_this_pointer_p (ptr))
    return false;
  tree saved = build1_loc (loc, SAVE_EXPR, TREE_TYPE (ptr), ptr);
  TREE_SIDE_EFFECTS (saved) = 1;
  hash_map<tree, tree> map;
  if (TREE_CODE (fn) != OBJ_TYPE_REF)
    {
      tree call
	= p3100_object_pointer_check
	    (loc, saved, "ub:expr.unary.dereference.nullptr.access");
      tree op = p3100_replace_operand (arg, ptr, saved, &map);
      *argp = build2 (COMPOUND_EXPR, TREE_TYPE (op), call, op);
      return true;
    }
  /* The object operand is evaluated first, then the vtable lookup, then
     the argument.  */
  fn = copy_node (fn);
  OBJ_TYPE_REF_OBJECT (fn)
    = p3100_replace_operand (OBJ_TYPE_REF_OBJECT (fn), ptr, saved, &map);
  tree call
    = p3100_object_pointer_check (loc, saved,
				  "ub:expr.unary.dereference.nullptr.access");
  tree expr = p3100_replace_operand (OBJ_TYPE_REF_EXPR (fn), ptr, saved,
				     &map);
  OBJ_TYPE_REF_EXPR (fn) = build2 (COMPOUND_EXPR, TREE_TYPE (expr), call,
				   expr);
  CALL_EXPR_FN (stmt) = fn;
  *argp = p3100_replace_operand (arg, ptr, saved, &map);
  return true;
}

/* P3100: STMT being a call of a virtual function by its unqualified name
   that the front end resolved statically (CALL_EXPR_P3100_VIRTUAL_P),
   whose object pointer is not the address of a subobject
   (cp_ubsan_maybe_check_virtual_call_object) -- `p->f ()' for a `final' f
   or a function of a `final' class, `p[1].f ()' -- instrument its object
   pointer as a member call's, but under the .access leaf of
   {expr.unary.dereference.nullptr}: the leaf of a virtual call follows the
   source, not whether the vtable pointer is read (DECISIONS.md E110 (a)).
   Return true if STMT is such a call: its object pointer is then not
   checked again.  */

bool
cp_ubsan_maybe_check_devirtualized_call (tree stmt)
{
  if (!flag_contracts_p3100
      || !CALL_EXPR_P3100_VIRTUAL_P (stmt)
      || call_expr_nargs (stmt) == 0)
    return false;
  tree op = CALL_EXPR_ARG (stmt, 0);
  if (op == error_mark_node || !POINTER_TYPE_P (TREE_TYPE (op)))
    return false;
  if (tree r = ubsan_maybe_instrument_member_call_object (EXPR_LOCATION (stmt),
							  op, true))
    CALL_EXPR_ARG (stmt, 0) = r;
  return true;
}

/* P3100: STMT being the AGGR_INIT_EXPR of a member function call
   returning a class with a non-trivial copy constructor or destructor by
   value, `p->f ()', `p[1].f ()', instrument its object pointer p for the
   P3100 null check, as ubsan_maybe_instrument_member_call does a
   CALL_EXPR's, which cp_genericize_r instruments alone: under
   .indirection, or under .access for a virtual function called by its
   unqualified name and resolved statically (CALL_EXPR_P3100_VIRTUAL_P,
   DECISIONS.md E110 (a)), also on a subobject of `*p'.  A call through the
   vtable reads the vtable pointer through p, which the ubsan pass checks
   (.access), as it checks the address of another subobject of `*p'
   (.indirection); the address of an object, `this', and a reference are
   not checked, nor
   is the object pointer for -fsanitize=null: its miss of such a call is
   stock (DECISIONS.md E113 (b)).  GCC-854.  */

void
cp_ubsan_maybe_check_aggr_init_call (tree stmt)
{
  if (!flag_contracts_p3100
      || processing_template_decl
      || current_function_decl == NULL_TREE
      || AGGR_INIT_VIA_CTOR_P (stmt)
      || aggr_init_expr_nargs (stmt) == 0)
    return;
  tree fn = AGGR_INIT_EXPR_FN (stmt);
  if (fn == NULL_TREE
      || TREE_CODE (fn) == OBJ_TYPE_REF
      || !INDIRECT_TYPE_P (TREE_TYPE (fn))
      || TREE_CODE (TREE_TYPE (TREE_TYPE (fn))) != METHOD_TYPE)
    return;
  tree *opp = &AGGR_INIT_EXPR_ARG (stmt, 0);
  tree op = *opp;
  if (op == error_mark_node || !TYPE_PTR_P (TREE_TYPE (op)))
    return;
  location_t loc = EXPR_LOCATION (stmt);
  /* A virtual function resolved statically on a subobject of `*p', `p->m.f
     ()' (m of a `final' class): check p under .access, as
     cp_ubsan_maybe_check_virtual_call_object does a CALL_EXPR's, the kind
     the object pointer of OP->*PM gets, so the ubsan pass does not check
     the subobject's address again.  */
  if (CALL_EXPR_P3100_VIRTUAL_P (stmt)
      && TREE_CODE (tree_strip_nop_conversions (op)) == ADDR_EXPR)
    {
      bool access = false;
      tree side = NULL_TREE;
      tree ptr = p3100_object_pointer (op, &access, &side);
      if (ptr != NULL_TREE)
	if (tree sub = ubsan_p3100_constant_subscript (ptr, 0))
	  ptr = ubsan_p3100_subscript_pointer (sub);
      if (!p3100_object_pointer_check_p (loc, ptr)
	  || cp_ubsan_this_pointer_p (ptr))
	return;
      tree saved = build1_loc (loc, SAVE_EXPR, TREE_TYPE (ptr), ptr);
      TREE_SIDE_EFFECTS (saved) = 1;
      hash_map<tree, tree> map;
      tree call
	= p3100_object_pointer_check
	    (loc, saved, "ub:expr.unary.dereference.nullptr.access");
      op = p3100_replace_operand (op, ptr, saved, &map);
      *opp = build2 (COMPOUND_EXPR, TREE_TYPE (op), call, op);
      return;
    }
  tree sub = ubsan_p3100_constant_subscript (op, 0);
  tree ptr = sub ? TREE_OPERAND (sub, 0) : op;
  tree stripped = tree_strip_nop_conversions (ptr);
  if (!TYPE_PTR_P (TREE_TYPE (stripped))
      || TREE_CODE (stripped) == ADDR_EXPR
      || TREE_CODE (stripped) == POINTER_PLUS_EXPR
      || CONSTANT_CLASS_P (stripped)
      || TREE_CODE (stripped) == SAVE_EXPR
      || cp_ubsan_this_pointer_p (stripped))
    return;
  tree saved = build1_loc (loc, SAVE_EXPR, TREE_TYPE (ptr), ptr);
  TREE_SIDE_EFFECTS (saved) = 1;
  tree check
    = ubsan_build_p3100_indirection_check (loc, saved,
					   CALL_EXPR_P3100_VIRTUAL_P (stmt));
  if (check == NULL_TREE)
    return;
  if (sub)
    TREE_OPERAND (sub, 0) = saved;
  else
    op = saved;
  *opp = build2 (COMPOUND_EXPR, TREE_TYPE (op), check, op);
}

/* P3100: ARG, a reference, is the source of a trivial copy or move of an
   empty class, which the front end elides: the copy reads nothing, so an
   indirection it copies from is not accessed.  If the glvalue ARG refers
   to is an indirection through a pointer, `E e2 = *ep', `e = *ep', also
   through std::move and the like, and the .indirection leaf of
   {expr.unary.dereference.nullptr} applies at LOC, return the void check
   that the pointer is not null, which evaluates every side effect of ARG
   but the cast calls; otherwise NULL_TREE.  The caller drops ARG, which
   the elided copy would otherwise drop with its reference binding and
   that binding's check.  */

tree
cp_ubsan_maybe_check_empty_copy_source (location_t loc, tree arg)
{
  if (!flag_contracts_p3100 || arg == NULL_TREE || arg == error_mark_node)
    return NULL_TREE;
  while (true)
    {
      arg = tree_strip_any_location_wrapper (arg);
      /* Conversions of a reference (to a reference or pointer).  */
      tree inner = arg;
      while (CONVERT_EXPR_P (inner)
	     && INDIRECT_TYPE_P (TREE_TYPE (TREE_OPERAND (inner, 0))))
	inner = TREE_OPERAND (inner, 0);
      if (inner != arg && TYPE_REF_P (TREE_TYPE (inner)))
	arg = inner;
      else if (p3100_cast_call_p (arg))
	arg = CALL_EXPR_ARG (arg, 0);
      else
	break;
    }
  if (!TYPE_REF_P (TREE_TYPE (arg)))
    return NULL_TREE;
  return cp_ubsan_maybe_check_unaccessed_glvalue (loc,
						  convert_from_reference (arg));
}

/* P3100: true if BASE is an empty class that is a non-virtual base class
   of the class DERIVED (other than DERIVED itself).  */

static bool
p3100_empty_base_of_p (tree base, tree derived)
{
  if (!CLASS_TYPE_P (base)
      || !CLASS_TYPE_P (derived)
      || !COMPLETE_TYPE_P (derived)
      || same_type_ignoring_top_level_qualifiers_p (base, derived)
      || !is_really_empty_class (base, /*ignore_vptr*/true))
    return false;
  base_kind kind;
  tree binfo = lookup_base (derived, base, ba_any, &kind, tf_none);
  return (binfo != NULL_TREE
	  && binfo != error_mark_node
	  && kind == bk_proper_base);
}

/* P3100: E is the glvalue an elided trivial copy or assignment of an
   empty class copies from or stores to, or a reference or pointer to it
   (the copy constructor's argument, through std::move and the like).  If
   it is an element of an array of known bound, `ea[i]', and the bounds
   checks of ub:expr.add.out.of.bounds.known.subscript or
   ub:expr.unary.dereference.invalid.known apply at LOC, return the void
   expression that forms its address, `&ea[i]', which is checked as such
   (ubsan_maybe_instrument_array_ref): the copy reads or writes no object,
   so the element one past the end is the indirection past the end,
   .invalid.known, as `&ea[N]' is, and one further past is
   .known.subscript (DECISIONS.md E95 (a)).  Otherwise NULL_TREE.  The
   element's index is saved, in place, so that the elided copy's other
   uses of it evaluate it once.  The expression is a run-time-only guard
   (COND_EXPR_IMPLICIT_UB_RUNTIME_P), which constant evaluation does not
   evaluate.  */

tree
cp_ubsan_maybe_check_empty_copy_bounds (location_t loc, tree e)
{
  if (!flag_contracts_p3100
      || processing_template_decl
      || cp_unevaluated_operand
      || current_function_decl == NULL_TREE
      || e == NULL_TREE
      || e == error_mark_node)
    return NULL_TREE;
  while (true)
    {
      e = tree_strip_any_location_wrapper (e);
      /* Conversions of a reference or pointer to the same class, or to an
	 empty non-virtual base class at offset zero (DECISIONS.md E98
	 (a)).  */
      if ((CONVERT_EXPR_P (e) || TREE_CODE (e) == NON_LVALUE_EXPR)
	  && INDIRECT_TYPE_P (TREE_TYPE (e))
	  && INDIRECT_TYPE_P (TREE_TYPE (TREE_OPERAND (e, 0)))
	  && ((same_type_ignoring_top_level_qualifiers_p
	       (TREE_TYPE (TREE_TYPE (e)),
		TREE_TYPE (TREE_TYPE (TREE_OPERAND (e, 0)))))
	      || p3100_empty_base_of_p (TREE_TYPE (TREE_TYPE (e)),
					TREE_TYPE (TREE_TYPE
						   (TREE_OPERAND (e, 0))))))
	e = TREE_OPERAND (e, 0);
      /* The address of an empty non-virtual base class at a nonzero
	 offset, `p + offset' (build_base_path).  */
      else if (TREE_CODE (e) == POINTER_PLUS_EXPR
	       && TREE_CODE (TREE_OPERAND (e, 1)) == INTEGER_CST
	       && p3100_empty_base_of_p (TREE_TYPE (TREE_TYPE (e)),
					 TREE_TYPE (TREE_TYPE
						    (TREE_OPERAND (e, 0)))))
	e = TREE_OPERAND (e, 0);
      else if (p3100_cast_call_p (e))
	e = CALL_EXPR_ARG (e, 0);
      else if (INDIRECT_REF_P (e) || TREE_CODE (e) == ADDR_EXPR)
	e = TREE_OPERAND (e, 0);
      else
	{
	  tree p = maybe_undo_parenthesized_ref (e);
	  if (p == e)
	    break;
	  e = p;
	}
    }
  if (TREE_CODE (e) != ARRAY_REF
      || TREE_CODE (TREE_TYPE (TREE_OPERAND (e, 0))) != ARRAY_TYPE)
    return NULL_TREE;
  if ((resolve_implicit_contract_semantic
       (current_function_decl, loc,
	"ub:expr.unary.dereference.invalid.known") == CES_ASSUME)
      && (resolve_implicit_contract_semantic
	  (current_function_decl, loc,
	   "ub:expr.add.out.of.bounds.known.subscript") == CES_ASSUME))
    return NULL_TREE;
  if (TREE_SIDE_EFFECTS (TREE_OPERAND (e, 1)))
    TREE_OPERAND (e, 1) = save_expr (TREE_OPERAND (e, 1));
  tree addr = build1_loc (loc, ADDR_EXPR, build_pointer_type (TREE_TYPE (e)),
			  e);
  tree use = build1_loc (loc, CONVERT_EXPR, void_type_node, addr);
  TREE_SIDE_EFFECTS (use) = 1;
  tree guard = build3_loc (loc, COND_EXPR, void_type_node, boolean_true_node,
			   use, void_node);
  COND_EXPR_IMPLICIT_UB_GUARD_P (guard) = true;
  COND_EXPR_IMPLICIT_UB_RUNTIME_P (guard) = true;
  TREE_SIDE_EFFECTS (guard) = 1;
  return guard;
}

/* P3100: walk_tree callback for p3100_check_pmf_vtable_read: the
   indirection through DATA, the sum `p + delta' a call through a pointer
   to member function shares with its vtable lookup.  */

static tree
p3100_pmf_vtable_read_r (tree *tp, int *walk_subtrees, void *data)
{
  if (INDIRECT_REF_P (*tp)
      && tree_strip_nop_conversions (TREE_OPERAND (*tp, 0)) == (tree) data)
    return *tp;
  if (TYPE_P (*tp))
    *walk_subtrees = 0;
  return NULL_TREE;
}

/* P3100: STMT being a call through a pointer to member function whose
   `this' argument is SUM, `obj + delta', shared with the vtable lookup of
   a virtual one (get_member_function_from_ptrfunc): the lookup reads the
   vtable pointer through SUM, which is not null for a null object pointer
   when the adjustment is not zero, nor for the element at a constant
   subscript.  Reading the vtable pointer is the .access leaf of
   {expr.unary.dereference.nullptr} (DECISIONS.md E102 (a)): if it applies,
   check PTR, the object pointer (p of `p[c]'), saved anew, before the read,
   and read through the address formed from it, `PTR + OFF + delta' (OFF
   the constant subscript's offset, or NULL_TREE).  The check is the kind
   the object pointer of OP->*PM gets (UBSAN_MEMBER_ACCESS), so the ubsan
   pass does not check the read again (ubsan_ptrmem_derived_p).  A
   non-virtual pointer to member function reads nothing and is unchanged.
   GCC-837.  */

static void
p3100_check_pmf_vtable_read (tree stmt, tree sum, tree ptr, tree off)
{
  tree fn = CALL_EXPR_FN (stmt);
  tree ref = walk_tree_without_duplicates (&fn, p3100_pmf_vtable_read_r,
					   sum);
  if (ref == NULL_TREE)
    return;
  tree stripped = tree_strip_nop_conversions (ptr);
  if (TREE_CODE (stripped) == TARGET_EXPR)
    ptr = TARGET_EXPR_SLOT (stripped);
  else if (TREE_CODE (stripped) == ADDR_EXPR
	   || CONSTANT_CLASS_P (stripped)
	   || is_this_parameter (stripped))
    return;
  location_t loc = EXPR_LOCATION (stmt);
  const char *group = "ub:expr.unary.dereference.nullptr.access";
  int reaction = resolve_implicit_ub_reaction (current_function_decl, loc,
					       group);
  if (reaction == IMPLICIT_UB_NONE)
    return;
  tree saved = build1_loc (loc, SAVE_EXPR, TREE_TYPE (ptr), ptr);
  TREE_SIDE_EFFECTS (saved) = 1;
  tree obj = saved;
  if (off)
    obj = build2 (POINTER_PLUS_EXPR, TREE_TYPE (saved), saved, off);
  obj = fold_convert (TREE_TYPE (TREE_OPERAND (sum, 0)), obj);
  tree addr = build2 (POINTER_PLUS_EXPR, TREE_TYPE (sum), obj,
		      TREE_OPERAND (sum, 1));
  tree call = p3100_object_pointer_check (loc, saved, group);
  tree type = TREE_TYPE (TREE_OPERAND (ref, 0));
  TREE_OPERAND (ref, 0) = build2 (COMPOUND_EXPR, type, call,
				  fold_convert (type, addr));
}

/* Data passed to p3100_pmf_vtable_cond_r.  */

struct p3100_pmf_vtable_cond_data
{
  /* The indirection through the vtable pointer.  */
  tree ref;
  /* The innermost conditional found with REF in one arm only.  */
  tree cond;
};

/* P3100: walk_tree callback for p3100_check_pmf_member_object: the
   conditional choosing between a pointer to member function's own
   function pointer and its vtable lookup, which has the indirection
   through the vtable pointer in one arm (not a guard evaluating the whole
   choice in both).  */

static tree
p3100_pmf_vtable_cond_r (tree *tp, int *walk_subtrees, void *data_)
{
  p3100_pmf_vtable_cond_data *data = (p3100_pmf_vtable_cond_data *) data_;
  if (TREE_CODE (*tp) == COND_EXPR
      && TREE_OPERAND (*tp, 1) != NULL_TREE
      && (find_tree (TREE_OPERAND (*tp, 1), data->ref)
	  != find_tree (TREE_OPERAND (*tp, 2), data->ref)))
    data->cond = *tp;
  if (TYPE_P (*tp))
    *walk_subtrees = 0;
  return NULL_TREE;
}

/* P3100: walk_tree callback for p3100_check_pmf_member_object: the comma
   expression whose first operand is DATA, a null check.  */

static tree
p3100_check_comma_r (tree *tp, int *walk_subtrees, void *data)
{
  if (TREE_CODE (*tp) == COMPOUND_EXPR && TREE_OPERAND (*tp, 0) == (tree) data)
    return *tp;
  if (TYPE_P (*tp))
    *walk_subtrees = 0;
  return NULL_TREE;
}

/* P3100: STMT being a call through a pointer to member function whose
   `this' argument is SUM, `obj + delta', shared with the vtable lookup of
   a virtual one, and TARG the TARGET_EXPR obj saves the object pointer in:
   if that is the address of a member subobject of the object a pointer p
   points to -- `(p->m.*pmf) ()', `(p->a[1].*pmf) ()', also through a
   pointer to data member, `((p->*pm).*pmf) ()' -- the lookup reads the
   vtable pointer through p, which is not the pointer it reads through, nor
   the one the ubsan pass checks the member's address on (.indirection).
   Check p once, after the pointer to member function is evaluated, under
   the leaf its virtual bit selects (DECISIONS.md E102 (a)): .access before
   the vtable lookup, .indirection where the function pointer is its own.
   p is saved where the object pointer is evaluated (the saved object
   pointer of OP->*PM, whose own check is dropped); the checks are the
   kind that object pointer's is, so the ubsan pass checks no address
   formed from p again (ubsan_ptrmem_derived_p), for -fsanitize=null
   neither (DECISIONS.md E111 (a)).  A member of a virtual base of `*p' is
   checked on p once, under .access, where the object pointer is evaluated
   (DECISIONS.md E94 (b)).  Return true if STMT is such a call.
   GCC-846.  */

static bool
p3100_check_pmf_member_object (tree stmt, tree sum, tree targ)
{
  tree fn = CALL_EXPR_FN (stmt);
  tree ref = walk_tree_without_duplicates (&fn, p3100_pmf_vtable_read_r,
					   sum);
  if (ref == NULL_TREE)
    return false;
  p3100_pmf_vtable_cond_data cdata = { ref, NULL_TREE };
  walk_tree_without_duplicates (&fn, p3100_pmf_vtable_cond_r, &cdata);
  tree cond = cdata.cond;
  if (cond == NULL_TREE)
    return false;
  location_t loc = EXPR_LOCATION (stmt);
  tree init = TARGET_EXPR_INITIAL (targ);
  tree saved;
  if (tree pcall = cp_ubsan_ptrmem_object_check (init))
    {
      /* `(p->*pm).*pmf': the object pointer of OP->*PM is saved, and
	 checked under .indirection; check it here instead.  */
      saved = CALL_EXPR_ARG (pcall, UBSAN_NULL_PTR);
      if (TREE_CODE (saved) != SAVE_EXPR
	  || cp_ubsan_this_pointer_p (saved)
	  || !p3100_object_pointer_check_p (loc, TREE_OPERAND (saved, 0)))
	return false;
      /* Drop it: it would check the pointer for -fsanitize=null as well,
	 which the checks below do (DECISIONS.md E111 (a)).  */
      tree comma = walk_tree_without_duplicates (&init, p3100_check_comma_r,
						 pcall);
      if (comma == NULL_TREE)
	return false;
      hash_map<tree, tree> map;
      TARGET_EXPR_INITIAL (targ)
	= p3100_replace_operand (init, comma, TREE_OPERAND (comma, 1), &map);
    }
  else
    {
      if (TREE_CODE (tree_strip_nop_conversions (init)) != ADDR_EXPR)
	return false;
      bool access = false;
      tree side = NULL_TREE;
      tree ptr = p3100_object_pointer (init, &access, &side);
      if (ptr != NULL_TREE)
	if (tree sub = ubsan_p3100_constant_subscript (ptr, 0))
	  ptr = ubsan_p3100_subscript_pointer (sub);
      if (!p3100_object_pointer_check_p (loc, ptr)
	  || cp_ubsan_this_pointer_p (ptr))
	return false;
      saved = build1_loc (loc, SAVE_EXPR, TREE_TYPE (ptr), ptr);
      TREE_SIDE_EFFECTS (saved) = 1;
      hash_map<tree, tree> map;
      init = p3100_replace_operand (init, ptr, saved, &map);
      if (access)
	{
	  /* A member of a virtual base, `(p->vb.m.*pmf) ()': the conversion
	     to the virtual base reads its offset through p where the object
	     pointer is evaluated, before the pointer to member function, the
	     .access leaf whatever the pointer to member function is
	     (DECISIONS.md E94 (b)); check p there.  GCC-850.  */
	  tree acc
	    = p3100_object_pointer_check
		(loc, saved, "ub:expr.unary.dereference.nullptr.access");
	  TARGET_EXPR_INITIAL (targ) = build2 (COMPOUND_EXPR, TREE_TYPE (init),
					       acc, init);
	  return true;
	}
      TARGET_EXPR_INITIAL (targ) = init;
    }
  tree acc
    = p3100_object_pointer_check (loc, saved,
				  "ub:expr.unary.dereference.nullptr.access");
  tree ind
    = p3100_object_pointer_check
	(loc, saved, "ub:expr.unary.dereference.nullptr.indirection");
  tree type = TREE_TYPE (TREE_OPERAND (ref, 0));
  TREE_OPERAND (ref, 0) = build2 (COMPOUND_EXPR, type, acc,
				  TREE_OPERAND (ref, 0));
  int own = find_tree (TREE_OPERAND (cond, 1), ref) ? 2 : 1;
  TREE_OPERAND (cond, own) = build2 (COMPOUND_EXPR,
				     TREE_TYPE (TREE_OPERAND (cond, own)),
				     ind, TREE_OPERAND (cond, own));
  return true;
}

/* P3100: STMT being a call through a pointer to member function whose
   `this' argument is SUM, `obj + delta', shared with the vtable lookup of
   a virtual one: if OBJ is the address of a base class subobject of the
   object a pointer p points to, saved in a TARGET_EXPR -- `(p->*pmf) ()',
   `((*p).*pmf) ()' with pmf a pointer to member function of a base, at
   offset zero or not, on an element at a constant subscript `p[1]' too
   (a base of a member subobject is p3100_check_pmf_member_object's) --
   the lookup reads the vtable pointer through p, and the ubsan pass would
   check the base's address on p under .indirection, the vtable pointer's
   read on `obj + delta' (not null for a base at a nonzero offset) and,
   with -fsanitize=null, the member call's object pointer again.  Check p once, after the pointer to member
   function is evaluated, under the leaf its virtual bit selects
   (DECISIONS.md E102 (a)), as p3100_check_pmf_member_object does; the
   checks are the kind the object pointer of OP->*PM gets, so the ubsan
   pass checks no address formed from p again (ubsan_ptrmem_derived_p),
   for -fsanitize=null neither (DECISIONS.md E111 (a)).  Return true if
   STMT is such a call.  GCC-864.  */

static bool
p3100_check_pmf_base_object (tree stmt, tree sum, tree obj)
{
  tree addr = tree_strip_nop_conversions (obj);
  if (TREE_CODE (addr) != ADDR_EXPR
      || TREE_CODE (TREE_OPERAND (addr, 0)) != COMPONENT_REF)
    return false;
  tree e = TREE_OPERAND (addr, 0);
  while (TREE_CODE (e) == COMPONENT_REF)
    {
      tree field = TREE_OPERAND (e, 1);
      if (TREE_CODE (field) != FIELD_DECL || !DECL_FIELD_IS_BASE (field))
	return false;
      e = TREE_OPERAND (e, 0);
    }
  if (!INDIRECT_REF_P (e) || REFERENCE_REF_P (e))
    return false;
  tree targ = tree_strip_nop_conversions (TREE_OPERAND (e, 0));
  if (TREE_CODE (targ) != TARGET_EXPR
      || TARGET_EXPR_INITIAL (targ) == NULL_TREE)
    return false;
  tree init = tree_strip_nop_conversions (TARGET_EXPR_INITIAL (targ));
  /* A base of a member subobject, `(p->m.*pmf) ()', `((p->*pm).*pmf) ()'
     for a pmf of a base of m's class.  */
  if (TREE_CODE (init) == ADDR_EXPR
      || cp_ubsan_ptrmem_object_check (TARGET_EXPR_INITIAL (targ)))
    return p3100_check_pmf_member_object (stmt, sum, targ);
  if (!TYPE_PTR_P (TREE_TYPE (init))
      || CONSTANT_CLASS_P (init)
      || cp_ubsan_this_pointer_p (init))
    return false;
  /* The element at a constant subscript, `(p[1].*pmf) ()', is checked on
     p (DECISIONS.md E97 (c)), saved where the object pointer is
     evaluated.  */
  tree sub = ubsan_p3100_constant_subscript (TARGET_EXPR_INITIAL (targ), 0);
  tree ptr = sub ? TREE_OPERAND (sub, 0) : TARGET_EXPR_SLOT (targ);
  location_t loc = EXPR_LOCATION (stmt);
  if (!p3100_object_pointer_check_p (loc, ptr))
    return false;
  tree fn = CALL_EXPR_FN (stmt);
  tree ref = walk_tree_without_duplicates (&fn, p3100_pmf_vtable_read_r,
					   sum);
  if (ref == NULL_TREE)
    return false;
  p3100_pmf_vtable_cond_data cdata = { ref, NULL_TREE };
  walk_tree_without_duplicates (&fn, p3100_pmf_vtable_cond_r, &cdata);
  tree cond = cdata.cond;
  if (cond == NULL_TREE)
    return false;
  if (sub)
    {
      ptr = build1_loc (loc, SAVE_EXPR, TREE_TYPE (ptr), ptr);
      TREE_SIDE_EFFECTS (ptr) = 1;
      TREE_OPERAND (sub, 0) = ptr;
    }
  tree acc
    = p3100_object_pointer_check (loc, ptr,
				  "ub:expr.unary.dereference.nullptr.access");
  tree ind
    = p3100_object_pointer_check
	(loc, ptr, "ub:expr.unary.dereference.nullptr.indirection");
  tree type = TREE_TYPE (TREE_OPERAND (ref, 0));
  TREE_OPERAND (ref, 0) = build2 (COMPOUND_EXPR, type, acc,
				  TREE_OPERAND (ref, 0));
  int own = find_tree (TREE_OPERAND (cond, 1), ref) ? 2 : 1;
  TREE_OPERAND (cond, own) = build2 (COMPOUND_EXPR,
				     TREE_TYPE (TREE_OPERAND (cond, own)),
				     ind, TREE_OPERAND (cond, own));
  return true;
}

/* P3100: STMT being a call of a member function through a pointer to
   member function, `(p->*pmf) ()', whose `this' argument
   get_member_function_from_ptrfunc forms as the object pointer plus the
   pointer to member function's adjustment, `p + delta', check the object
   pointer before the adjustment instead: `p + delta' is not null for a
   null p when the adjustment is not zero (the offset of a second base,
   GCC-835).  The object pointer of the element at a constant subscript,
   `(sp[1].*pmf) ()', or at a constant offset, `((sp + 1)->*pmf) ()', saved
   in a TARGET_EXPR, is checked on sp (DECISIONS.md E97 (c), E99 (a);
   GCC-834), its pointer saved where the object pointer is evaluated.
   The vtable lookup of a virtual pointer to member function, which reads
   through `p + delta' first, checks the object pointer under .access
   (p3100_check_pmf_vtable_read).  Return true if STMT is such a call: its
   `this' is then not checked again.  */

bool
cp_ubsan_maybe_instrument_ptrmemfunc_call (tree stmt)
{
  if (!flag_contracts_p3100 || call_expr_nargs (stmt) == 0)
    return false;
  tree sum = CALL_EXPR_ARG (stmt, 0);
  while (CONVERT_EXPR_P (sum))
    sum = TREE_OPERAND (sum, 0);
  if (TREE_CODE (sum) != POINTER_PLUS_EXPR)
    return false;
  tree delta = TREE_OPERAND (sum, 1);
  STRIP_NOPS (delta);
  if (TREE_CODE (delta) == RSHIFT_EXPR)
    {
      delta = TREE_OPERAND (delta, 0);
      STRIP_NOPS (delta);
    }
  if (TREE_CODE (delta) != COMPONENT_REF
      || !TYPE_PTRMEMFUNC_P (TREE_TYPE (TREE_OPERAND (delta, 0))))
    return false;
  /* The sum is shared with the vtable lookup of a virtual pointer to
     member function, which reads through it before `this' is evaluated
     (and reports a null object pointer itself, .access): rebuild it for
     the argument alone.  */
  tree *argp = &CALL_EXPR_ARG (stmt, 0);
  while (true)
    {
      *argp = copy_node (*argp);
      if (TREE_CODE (*argp) == POINTER_PLUS_EXPR)
	break;
      argp = &TREE_OPERAND (*argp, 0);
    }
  tree *objp = &TREE_OPERAND (*argp, 0);
  tree obj = *objp;
  location_t loc = EXPR_LOCATION (stmt);
  tree targ = tree_strip_nop_conversions (obj);
  if (TREE_CODE (targ) == TARGET_EXPR
      && TARGET_EXPR_INITIAL (targ) != NULL_TREE)
    if (tree sub = ubsan_p3100_constant_subscript (TARGET_EXPR_INITIAL (targ),
						   0))
      {
	/* Save sp where the object pointer is evaluated, and check it
	   here.  */
	tree ptr = save_expr (TREE_OPERAND (sub, 0));
	TREE_OPERAND (sub, 0) = ptr;
	p3100_check_pmf_vtable_read (stmt, sum, ptr, TREE_OPERAND (sub, 1));
	if (tree check = ubsan_maybe_instrument_member_call_object (loc, ptr))
	  *objp = build2 (COMPOUND_EXPR, TREE_TYPE (obj), check, obj);
	return true;
      }
  if (TREE_CODE (targ) == TARGET_EXPR
      && TARGET_EXPR_INITIAL (targ) != NULL_TREE
      && p3100_check_pmf_member_object (stmt, sum, targ))
    return true;
  if (p3100_check_pmf_base_object (stmt, sum, obj))
    return true;
  p3100_check_pmf_vtable_read (stmt, sum, obj, NULL_TREE);
  if (tree check = ubsan_maybe_instrument_member_call_object (loc, obj))
    *objp = check;
  return true;
}

/* P3100: true if cp_fold should fold CALL, a call of std::move,
   std::forward and the like, to the cast of its argument even where it
   does not by default (-O0): when the null check of
   {expr.unary.dereference.nullptr} applies at the call, so that the
   reference binding of `*p' to the parameter and a read through the
   result, `s2 = std::move (*sp)', are seen as the one dereference they
   are, reported once.  */

bool
cp_ubsan_fold_cast_call_p (tree call)
{
  if (!flag_contracts_p3100
      || current_function_decl == NULL_TREE
      || !p3100_cast_call_p (call))
    return false;
  location_t loc = cp_expr_loc_or_input_loc (call);
  return ((resolve_implicit_ub_reaction
	   (current_function_decl, loc,
	    "ub:expr.unary.dereference.nullptr.indirection")
	   != IMPLICIT_UB_NONE)
	  || (resolve_implicit_ub_reaction
	      (current_function_decl, loc,
	       "ub:expr.unary.dereference.nullptr.access")
	      != IMPLICIT_UB_NONE));
}

/* P3100: true if the call CALL of std::addressof or std::__addressof has
   an argument that binds `*p' to the reference parameter, which
   ubsan_maybe_instrument_reference checks when it instruments the
   function under the .indirection leaf: cp_fold must not fold the call
   to the conversion of its argument, which drops that check, when the
   leaf applies at the call.  */

bool
cp_ubsan_keep_addressof_call_p (tree call)
{
  if (!flag_contracts_p3100
      || current_function_decl == NULL_TREE
      || call_expr_nargs (call) != 1)
    return false;
  tree arg = tree_strip_any_location_wrapper (CALL_EXPR_ARG (call, 0));
  if (TREE_CODE (arg) != NOP_EXPR || !TYPE_REF_P (TREE_TYPE (arg)))
    return false;
  tree ptr = tree_strip_nop_conversions (TREE_OPERAND (arg, 0));
  if (!TYPE_PTR_P (TREE_TYPE (ptr))
      || TREE_CODE (ptr) == ADDR_EXPR
      || is_this_parameter (ptr))
    return false;
  return (resolve_implicit_ub_reaction
	  (current_function_decl, cp_expr_loc_or_input_loc (call),
	   "ub:expr.unary.dereference.nullptr.indirection")
	  != IMPLICIT_UB_NONE);
}
