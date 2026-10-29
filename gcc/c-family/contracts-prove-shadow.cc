/* Proving strategies for the P4405 prove and diagnose evaluation semantics:
   proof shadows, the front-end half.  See contracts-prove.h, and
   contracts-shadow.cc for the middle end, which does the proving.

   Copyright (C) 2026 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* The strategy cannot answer where the evaluation is described: the proof
   is the optimizers' work on a copy of the function.  So it defers every
   deferrable evaluation of a function that will be optimized, and the front
   end places a region for it where the check would have been
   (shadow_mark); earlier evaluations that the shadow may assume get one
   too (shadow_assume).  The regions are only
   ever executed in the shadow, which is never emitted: the predicate is
   never evaluated at run time, and the real function's code is ignore's.

   Shadows are not active without optimization -- at -O0 folding is final
   -- nor under -fsyntax-only, while writing a precompiled header (the
   regions would outlive the compilation that registered them), with
   -fanalyzer (which would analyze the shadows as functions of their own),
   or with profile instrumentation or feedback (which would count or look
   up the shadows' own profiles).

   A caller-side precondition's region evaluates the predicate with each
   parameter it names replaced by the call's argument, which is then
   evaluated twice in the shadow -- in the region and in the call -- and
   once in the program, as before.  So every such argument must be free of
   side effects and of anything evaluated only once (a temporary, a saved
   expression, a statement); otherwise the evaluation is not deferred and
   only the front-end strategies decide it.

   Under -fcontracts-prove-assume the region of an obligation is followed by
   an assumption for the real function, `.ASSUME (!.CONTRACT_PROVEN (id) ||
   pred)' (see contracts-shadow.cc), when the predicate can be copied
   (nothing evaluated only once, as above).  The assumption is lowered as
   [[assume]]'s, outlined, so the predicate is never evaluated at run time
   either way.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tree.h"
#include "options.h"
#include "c-common.h"
#include "internal-fn.h"
#include "c-family/contracts-prove.h"
#include "contracts-shadow.h"

/* walk_tree callback for a caller-side predicate: copy it, replacing the
   parameters mapped in DATA, a hash_map<tree, tree>.  Only expression nodes
   are copied; declarations, types and constants are shared.  A parameter
   not mapped, or any node whose copy would not be a fresh evaluation of
   the same thing -- a temporary, a saved expression, a statement, a list,
   vector or constructor, or a front-end-specific node -- stops the walk.  */

static tree
shadow_substitute_r (tree *tp, int *walk_subtrees, void *data)
{
  tree t = *tp;
  enum tree_code code = TREE_CODE (t);
  if (code == PARM_DECL)
    {
      tree *v = static_cast<hash_map<tree, tree> *> (data)->get (t);
      if (!v)
	return t;
      *tp = unshare_expr (*v);
      *walk_subtrees = 0;
      return NULL_TREE;
    }
  if (code == BIND_EXPR || code == DECL_EXPR || code == TARGET_EXPR
      || code == SAVE_EXPR || code == STATEMENT_LIST
      || (int) code >= (int) LAST_AND_UNUSED_TREE_CODE)
    return t;
  if (IS_EXPR_CODE_CLASS (TREE_CODE_CLASS (code)))
    {
      *tp = copy_node (t);
      return NULL_TREE;
    }
  if (TREE_CODE_CLASS (code) == tcc_exceptional)
    return t;
  *walk_subtrees = 0;
  return NULL_TREE;
}

/* walk_tree callback: an argument that can be evaluated again has none of
   the nodes shadow_substitute_r stops at, other than parameters.  */

static tree
shadow_repeatable_r (tree *tp, int *walk_subtrees, void *)
{
  tree t = *tp;
  enum tree_code code = TREE_CODE (t);
  if (code == BIND_EXPR || code == DECL_EXPR || code == TARGET_EXPR
      || code == SAVE_EXPR || code == STATEMENT_LIST
      || (int) code >= (int) LAST_AND_UNUSED_TREE_CODE)
    return t;
  if (IS_EXPR_CODE_CLASS (TREE_CODE_CLASS (code)))
    return NULL_TREE;
  if (TREE_CODE_CLASS (code) == tcc_exceptional)
    return t;
  *walk_subtrees = 0;
  return NULL_TREE;
}

/* PRED, the predicate of QUERY, a caller-side precondition, with the call's
   arguments for the parameters it names; NULL_TREE if one of them cannot
   be evaluated again.  */

static tree
shadow_substitute (const contract_proof_query &query, tree pred)
{
  hash_map<tree, tree> map;
  int nargs = call_expr_nargs (query.call);
  int i = 0;
  for (tree p = query.parms; p && i < nargs; p = DECL_CHAIN (p), ++i)
    {
      tree arg = CALL_EXPR_ARG (query.call, i);
      if (arg != error_mark_node
	  && !TREE_SIDE_EFFECTS (arg)
	  && !walk_tree_without_duplicates (&arg, shadow_repeatable_r, NULL))
	map.put (p, arg);
    }
  tree copy = pred;
  if (walk_tree (&copy, shadow_substitute_r, &map, NULL))
    return NULL_TREE;
  return copy;
}

/* A second copy of PRED, the predicate of QUERY as its region evaluates it
   (for a caller-side precondition, ORIG is the predicate before the
   arguments replaced its parameters), for the assumption made of it;
   NULL_TREE if it holds something that cannot be evaluated twice.  */

static tree
shadow_copy (const contract_proof_query &query, tree orig, tree pred)
{
  if (query.call)
    return shadow_substitute (query, orig);
  if (walk_tree_without_duplicates (&pred, shadow_repeatable_r, NULL))
    return NULL_TREE;
  return unshare_expr (pred);
}

/* walk_tree callback collecting, in DATA, a vec<tree>, the functions an
   expression calls directly, and stopping at anything else that may have a
   side effect: a call that is indirect or to an internal function that may
   have one, or a node with side effects none of its operands has (a store,
   an increment, a volatile access, a front end's own such node).  */

static tree
shadow_effects_r (tree *tp, int *walk_subtrees, void *data)
{
  tree t = *tp;
  if (!TREE_SIDE_EFFECTS (t))
    {
      *walk_subtrees = 0;
      return NULL_TREE;
    }
  if (TREE_CODE (t) == CALL_EXPR)
    {
      if (CALL_EXPR_FN (t) == NULL_TREE)
	{
	  int flags = internal_fn_flags (CALL_EXPR_IFN (t));
	  if (!(flags & (ECF_CONST | ECF_PURE))
	      || (flags & ECF_LOOPING_CONST_OR_PURE))
	    return t;
	}
      else if (tree fn = get_callee_fndecl (t))
	static_cast<vec<tree> *> (data)->safe_push (fn);
      else
	return t;
      return NULL_TREE;
    }
  if (!EXPR_P (t))
    return t;
  for (int i = 0; i < TREE_OPERAND_LENGTH (t); ++i)
    if (TREE_OPERAND (t, i) && TREE_SIDE_EFFECTS (TREE_OPERAND (t, i)))
      return NULL_TREE;
  return t;
}

/* True if PRED may have side effects other than through calls to the
   functions it calls directly, which this pushes onto CALLEES.  */

static bool
shadow_effects (tree pred, vec<tree> &callees)
{
  return walk_tree_without_duplicates (&pred, shadow_effects_r, &callees);
}

/* True if FNDECL's evaluations may go to a proof shadow of either kind,
   whichever strategy's flag is set.  */

bool
contract_prove_shadow_may_defer_p (tree fndecl)
{
  return (fndecl
	  && TREE_CODE (fndecl) == FUNCTION_DECL
	  && !flag_syntax_only
	  && !pch_file
	  && !flag_analyzer
	  && !profile_arc_flag
	  && !flag_test_coverage
	  && !flag_branch_probabilities
	  && !flag_auto_profile
	  && opt_for_fn (fndecl, optimize) > 0);
}

/* True if FNDECL's evaluations may go to its proof shadow.  */

static bool
shadow_defers_p (tree fndecl)
{
  return (flag_contracts_prove_shadow
	  && contract_prove_shadow_may_defer_p (fndecl));
}

/* The region `if (.CONTRACT_SHADOW_GUARD (KIND)) { if (!PRED) TAIL; }' at
   LOC: KIND is 0 for an ordinary region and an override pair's number for
   one of that pair's override shadow.  */

static tree
shadow_region (location_t loc, tree pred, tree tail, unsigned kind)
{
  tree guard = build_call_expr_internal_loc (loc, IFN_CONTRACT_SHADOW_GUARD,
					     boolean_type_node, 1,
					     build_int_cst (unsigned_type_node,
							    kind));
  tree neg = build1_loc (loc, TRUTH_NOT_EXPR, TREE_TYPE (pred), pred);
  tree inner = build3_loc (loc, COND_EXPR, void_type_node, neg, tail,
			   build_empty_stmt (loc));
  return build3_loc (loc, COND_EXPR, void_type_node, guard, inner,
		     build_empty_stmt (loc));
}

/* The front end's context for the diagnostic of an evaluation it is
   describing now, which the middle end re-establishes through the front
   end's hook should the diagnostic be issued at the end.  */

static unsigned
shadow_context ()
{
  contract_shadow_diagnostic_context = c_contract_switch_diagnostic_context;
  return c_contract_diagnostic_context ();
}

/* The region for QUERY, which shadow_prove deferred, with the predicate
   PRED as a check would evaluate it where it is placed (for a caller-side
   precondition, in terms of the callee's parameters, which this replaces
   with the call's arguments).  */

static tree
shadow_mark (const contract_proof_query &query, tree pred)
{
  tree orig = pred;
  if (query.call)
    pred = shadow_substitute (query, pred);
  else
    pred = unshare_expr (pred);
  gcc_assert (pred);
  auto_vec<tree> callees;
  bool side_effects = shadow_effects (pred, callees);
  /* An override pair's evaluation made in the ordinary shadow is an
     ordinary obligation, but for its diagnostic.  */
  unsigned kind = query.override_shadow ? query.override_pair : 0;
  unsigned id
    = contract_shadow_register_obligation (current_function_decl, query.loc,
					   query.pred_loc,
					   query.semantic == CES_PROVE,
					   side_effects, callees,
					   shadow_context (),
					   query.override_pair, kind);
  tree idx = build_int_cst (unsigned_type_node, id);
  tree tail = build_call_expr_internal_loc (query.loc, IFN_CONTRACT_UNPROVEN,
					    void_type_node, 1, idx);
  tree region = shadow_region (query.loc, pred, tail, kind);
  tree copy;
  /* An override shadow's proof is made under the overridden function's
     preconditions, which the real function's other callers need not
     establish, and an overridden function's postcondition has no check
     in the overriding function: neither is ever assumed.  */
  if (!flag_contracts_prove_assume
      || query.override_pair
      || !(copy = shadow_copy (query, orig, pred)))
    return region;
  /* Nothing is assumed until the guard is folded to true; the call keeps
     the condition from being simple, which would make the gimplifier expand
     it in place rather than outline it.  */
  contract_shadow_note_proven_guard ();
  tree guard = build_call_expr_internal_loc (query.loc, IFN_CONTRACT_PROVEN,
					     boolean_type_node, 1, idx);
  tree cond = build2_loc (query.loc, TRUTH_ORIF_EXPR, boolean_type_node,
			  build1_loc (query.loc, TRUTH_NOT_EXPR,
				      boolean_type_node, guard),
			  fold_convert_loc (query.loc, boolean_type_node,
					    copy));
  tree assume = build_call_expr_internal_loc (query.loc, IFN_ASSUME,
					      void_type_node, 1, cond);
  return build2_loc (query.loc, COMPOUND_EXPR, void_type_node, region,
		     assume);
}

/* The region letting a shadow assume PRED, the predicate of an evaluation
   at LOC with semantic SEM, or NULL_TREE: none is needed after enforce's,
   noexcept_enforce's and quick_enforce's checks, a prove or diagnose
   evaluation is an obligation, and a predicate that may have side effects
   is never assumed.  KIND is the region's kind (shadow_region).  */

tree
contract_prove_shadow_assumption (tree pred, contract_evaluation_semantic sem,
				  location_t loc, unsigned kind)
{
  bool guaranteed;
  switch (sem)
    {
    case CES_ASSUME:
      guaranteed = true;
      break;
    case CES_IGNORE:
    case CES_OBSERVE:
    case CES_NOEXCEPT_OBSERVE:
      guaranteed = false;
      break;
    default:
      /* enforce's and noexcept_enforce's checks call a noreturn violation
	 entry point and quick_enforce's traps, so they already ensure the
	 predicate on the continuing path, as code the optimizers see like
	 any other; prove and diagnose are obligations.  */
      return NULL_TREE;
    }
  if (!contract_prove_shadow_may_defer_p (current_function_decl)
      || !pred || pred == error_mark_node || TREE_SIDE_EFFECTS (pred))
    return NULL_TREE;
  contract_shadow_note_region ();
  tree tail = build_call_expr_internal_loc (loc, IFN_CONTRACT_ASSUMPTION,
					    void_type_node, 2,
					    build_int_cst (integer_type_node,
							   guaranteed),
					    build_int_cst (unsigned_type_node,
							   kind));
  return shadow_region (loc, unshare_expr (pred), tail, kind);
}

/* The region letting the ordinary shadow assume PRED (as
   contract_prove_shadow_assumption); an override pair's (PAIR) are the
   override shadows' (contracts-prove-override.cc).  */

static tree
shadow_assume (tree pred, contract_evaluation_semantic sem, location_t loc,
	       unsigned pair)
{
  if (pair)
    return NULL_TREE;
  return contract_prove_shadow_assumption (pred, sem, loc, 0);
}

/* True while proof shadows may be active in the translation unit: the
   diagnostics then all wait for the end of the compilation.  */

static bool
shadow_defers_diagnostics_p ()
{
  /* The command line's optimization level, not the current function's.  */
  int level = TREE_OPTIMIZATION (optimization_default_node)->x_optimize;
  return ((flag_contracts_prove_shadow || flag_contracts_prove_override)
	  && !flag_syntax_only
	  && !pch_file
	  && (level > 0
	      || shadow_defers_p (current_function_decl)
	      || contract_shadow_pending_p ()));
}

/* Issue the diagnostic of QUERY, decided now not to be proven, with the
   shadows' at the end of the compilation, while shadows may be active, so
   that an error now does not stop the compilation before they are
   optimized.  */

static bool
shadow_take_diagnostic (const contract_proof_query &query, bool always_false)
{
  /* An override pair's, at any optimization level and whether or not the
     shadows prove: issued only if its function is compiled (DECISIONS.md
     E150), which is known at the end.  */
  if (query.override_pair && !flag_syntax_only && !pch_file)
    {
      contract_shadow_defer_diagnostic (current_function_decl, query.loc,
					query.pred_loc,
					query.semantic == CES_PROVE,
					always_false, shadow_context (),
					query.override_pair);
      return true;
    }
  if (!shadow_defers_diagnostics_p ())
    return false;
  contract_shadow_defer_diagnostic (current_function_decl, query.loc,
				    query.pred_loc,
				    query.semantic == CES_PROVE, always_false,
				    shadow_context ());
  return true;
}

/* The body of FNDECL holds an evaluation whose code a shadow may take its
   predicate from, and which another translation unit's copy of FNDECL need
   not make: the middle end then requires FNDECL's body to have been
   inlined where a shadow relied on it (contracts-shadow.cc).  Any function
   may be inlined into a shadow, so this is recorded whatever FNDECL's own
   optimization level.  */

static void
shadow_note_premise (tree fndecl)
{
  if (fndecl && TREE_CODE (fndecl) == FUNCTION_DECL)
    contract_shadow_note_premise (fndecl);
}

static bool
shadow_enabled_p ()
{
  return flag_contracts_prove_shadow;
}

static contract_proof_result
shadow_prove (const contract_proof_query &query)
{
  /* An override shadow's evaluation is the override shadows' strategy's
     (contracts-prove-override.cc).  */
  if (!query.deferrable
      || (query.override_pair && query.override_shadow)
      || !shadow_defers_p (current_function_decl))
    return CPR_UNPROVEN;
  if (query.call && !shadow_substitute (query, query.predicate))
    return CPR_UNPROVEN;
  return CPR_DEFERRED;
}

const contract_proof_strategy contract_prove_shadow_strategy = {
  "shadow",
  /*may_depend_on_assumptions=*/true,
  shadow_enabled_p,
  shadow_prove,
  shadow_defers_p,
  shadow_mark,
  shadow_assume,
  shadow_take_diagnostic,
  shadow_note_premise
};
