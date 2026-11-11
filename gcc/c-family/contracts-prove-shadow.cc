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

   Shadows are optimized at the proof level, whatever the optimization
   level of the code (contract_shadow_level_p); at proof level 0, which
   enables no shadow, folding is final, and a shadow given explicitly there
   is optimized at level 0 (DECISIONS.md E208 (a)).  They are not active
   under -fsyntax-only, while writing a precompiled header (the
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

   An evaluation's region also evaluates the evaluations nested in it
   (DECISIONS.md E169, contract_proof_query's nested): the front end places
   their markers ahead of their calls in the region's copy of the
   predicate, as obligations nested in it (contracts-shadow.cc).  Those a
   front-end strategy decides at once are recorded with their results.

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
#include "tree-iterator.h"
#include "c-family/contracts-prove.h"
#include "contracts-shadow.h"
#include "diagnostic.h"

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

/* True unless QUERY is at a call whose arguments a marker cannot be given
   (shadow_substitute).  */

bool
contract_prove_shadow_substitutable_p (const contract_proof_query &query)
{
  return !query.call || shadow_substitute (query, query.predicate);
}

/* True if FNDECL's evaluations may go to a proof shadow of either kind,
   whichever strategy's flag is set.  The shadows are optimized at the proof
   level, so FNDECL's own optimize attribute does not matter (DECISIONS.md
   E168 (7)).  */

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
	  && contract_shadow_level_p ());
}

/* The optimization options of the proof shadows at proof level LEVEL
   (contract_shadow_proof_optimization): what an optimize attribute or
   pragma giving that level would make of the command line's options:
   those it gave are kept, and those the optimization level sets are the
   proof level's (DECISIONS.md E165).  The settings the command line's own
   level forced on what was not given -- -O0's no inlining, -O0's and
   -Og's trapping __builtin_unreachable, -Os's and -Og's RTL parameters --
   are reset first, so that only the proof level's apply.  Every option is
   restored afterwards, including those that are not per function, which
   the level sets too (--param max-fields-for-field-sensitive), and the
   target's: the code of the program must not change, and the
   analysis-only functions keep the command line's target options.  */

tree
c_contract_proof_optimization (int level)
{
  gcc_options saved = global_options;
  gcc_options saved_set = global_options_set;
  cl_target_option saved_target;
  cl_target_option_save (&saved_target, &global_options,
			 &global_options_set);
  cl_optimization_restore (&global_options, &global_options_set,
			   TREE_OPTIMIZATION (optimization_default_node));
  cl_target_option_restore (&global_options, &global_options_set,
			    TREE_TARGET_OPTION (target_option_default_node));
  if (!global_options_set.x_flag_no_inline)
    flag_no_inline = global_options_init.x_flag_no_inline;
  if (!global_options_set.x_flag_unreachable_traps)
    flag_unreachable_traps = global_options_init.x_flag_unreachable_traps;
  if (!global_options_set.x_param_min_crossjump_insns)
    param_min_crossjump_insns
      = global_options_init.x_param_min_crossjump_insns;
  if (!global_options_set.x_param_max_combine_insns)
    param_max_combine_insns = global_options_init.x_param_max_combine_insns;

  /* The command line's options were diagnosed already.  */
  bool inhibit = global_dc->m_inhibit_warnings;
  global_dc->m_inhibit_warnings = true;
  parse_optimize_options (build_tree_list (NULL_TREE,
					   build_int_cst (integer_type_node,
							  level)),
			  false);
  global_dc->m_inhibit_warnings = inhibit;
  /* Alignment is code generation's, which an analysis-only function never
     reaches; keep the command line's.  The alignment of the function
     compiled last stays in effect for the next one if that one sets none
     (parse_alignment_opts), so the proof level's would change the
     program's code (at -O1 and -Og).  */
  const cl_optimization *cmd = TREE_OPTIMIZATION (optimization_default_node);
  flag_align_functions = cmd->x_flag_align_functions;
  str_align_functions = cmd->x_str_align_functions;
  flag_align_jumps = cmd->x_flag_align_jumps;
  str_align_jumps = cmd->x_str_align_jumps;
  flag_align_labels = cmd->x_flag_align_labels;
  str_align_labels = cmd->x_str_align_labels;
  flag_align_loops = cmd->x_flag_align_loops;
  str_align_loops = cmd->x_str_align_loops;
  tree opts = build_optimization_node (&global_options, &global_options_set);

  global_options = saved;
  global_options_set = saved_set;
  cl_target_option_restore (&global_options, &global_options_set,
			    &saved_target);
  return opts;
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
   one of that pair's override shadow.  The guard of an obligation's region
   (OBLIGATION) has a second argument, 1, so that a premise copy, which
   executes the ordinary assumption regions only, can tell them apart
   (contracts-shadow.cc).  */

static tree
shadow_region (location_t loc, tree pred, tree tail, unsigned kind,
	       bool obligation)
{
  tree k = build_int_cst (unsigned_type_node, kind);
  tree guard
    = (obligation
       ? build_call_expr_internal_loc (loc, IFN_CONTRACT_SHADOW_GUARD,
				       boolean_type_node, 2, k,
				       integer_one_node)
       : build_call_expr_internal_loc (loc, IFN_CONTRACT_SHADOW_GUARD,
				       boolean_type_node, 1, k));
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
   with the call's arguments).  ALWAYS_FALSE if folding found PRED false
   wherever it is evaluated: the shadow then only decides whether the
   evaluation is reached (DECISIONS.md E195).  */

static tree
shadow_mark (const contract_proof_query &query, tree pred, bool always_false)
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
  /* E169: one nested in another obligation counts for that one.  */
  unsigned parent = query.nest ? query.nest->handle : 0;
  gcc_checking_assert (!query.nest || query.nest->strategy);
  unsigned id
    = contract_shadow_register_obligation (current_function_decl, query.loc,
					   query.pred_loc,
					   query.semantic == CES_PROVE,
					   side_effects, callees,
					   shadow_context (),
					   query.override_pair, kind, parent,
					   always_false);
  tree idx = build_int_cst (unsigned_type_node, id);
  /* An override shadow's proof is made under the overridden function's
     preconditions, which the real function's other callers need not
     establish, and an overridden function's postcondition has no check
     in the overriding function: neither is ever assumed.  Nor is a nested
     obligation: its marker is in the region of the one it is nested in,
     which the real function never evaluates.  The copy is made before the
     nested obligations' markers are placed in PRED.  */
  tree copy = NULL_TREE;
  if (flag_contracts_prove_assume && !query.override_pair && !parent)
    copy = shadow_copy (query, orig, pred);
  /* E169: the evaluations nested in this one, whose markers the region
     evaluates ahead of their calls in the predicate, as obligations nested
     in this one.  */
  if (query.nested)
    {
      contract_proof_nesting n = {};
      n.up = query.nest;
      n.strategy = &contract_prove_shadow_strategy;
      n.handle = id + 1;
      n.markers = true;
      pred = query.nested (query.nested_data, pred, &n);
    }
  tree tail = build_call_expr_internal_loc (query.loc, IFN_CONTRACT_UNPROVEN,
					    void_type_node, 1, idx);
  tree region;
  if (parent)
    {
      /* A side branch that never joins the path again
	 (CONTRACT_SHADOW_NESTED_KIND).  */
      tree trap = build_call_expr_loc (query.loc,
				       builtin_decl_explicit (BUILT_IN_TRAP),
				       0);
      tail = build2_loc (query.loc, COMPOUND_EXPR, void_type_node,
			 build3_loc (query.loc, COND_EXPR, void_type_node,
				     build1_loc (query.loc, TRUTH_NOT_EXPR,
						 TREE_TYPE (pred), pred),
				     tail, build_empty_stmt (query.loc)),
			 trap);
      tree guard
	= build_call_expr_internal_loc (query.loc, IFN_CONTRACT_SHADOW_GUARD,
					boolean_type_node, 1,
					build_int_cst
					  (unsigned_type_node,
					   CONTRACT_SHADOW_NESTED_KIND));
      region = build3_loc (query.loc, COND_EXPR, void_type_node, guard, tail,
			   build_empty_stmt (query.loc));
    }
  else
    region = shadow_region (query.loc, pred, tail, kind, true);
  if (!copy)
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

bool
contract_prove_shadow_assumable_p (tree pred, vec<tree> &callees)
{
  if (!pred || pred == error_mark_node)
    return false;
  if (!TREE_SIDE_EFFECTS (pred))
    return true;
  /* The region evaluates its own copy of the predicate, next to the
     check's: nothing in it may be evaluated only once.  */
  return (!shadow_effects (pred, callees)
	  && !walk_tree_without_duplicates (&pred, shadow_repeatable_r, NULL));
}

/* The region letting a shadow assume PRED, the predicate of an evaluation
   at LOC with semantic SEM, or NULL_TREE: none is needed after enforce's,
   noexcept_enforce's and quick_enforce's checks, a prove or diagnose
   evaluation is an obligation, and a predicate with side effects of its
   own, or holding something evaluated only once, is never assumed
   (contract_prove_shadow_assumable_p).  A predicate that calls functions
   is assumed, and the functions are registered with the region: a proof
   relying on it is conditional unless they are found free of side
   effects, terminating and not throwing (D4405R0,
   "Previous Assertions and Undefined Behavior"; contracts-shadow.cc).
   KIND is the region's kind (shadow_region).  */

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
  auto_vec<tree> callees;
  if (!contract_prove_shadow_may_defer_p (current_function_decl)
      || !contract_prove_shadow_assumable_p (pred, callees))
    return NULL_TREE;
  contract_shadow_note_region ();
  unsigned deps = (callees.is_empty () ? 0
		   : contract_shadow_register_assumption (callees));
  tree tail = build_call_expr_internal_loc (loc, IFN_CONTRACT_ASSUMPTION,
					    void_type_node, 3,
					    build_int_cst (integer_type_node,
							   guaranteed),
					    build_int_cst (unsigned_type_node,
							   kind),
					    build_int_cst (unsigned_type_node,
							   deps));
  return shadow_region (loc, unshare_expr (pred), tail, kind, false);
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
  return ((flag_contracts_prove_shadow || flag_contracts_prove_override)
	  && !flag_syntax_only
	  && !pch_file
	  && (contract_shadow_level_p ()
	      || contract_shadow_pending_p ()));
}

/* Issue the diagnostic of QUERY, decided now not to be proven, with the
   shadows' at the end of the compilation, while shadows may be active, so
   that an error now does not stop the compilation before they are
   optimized.  */

static bool
shadow_take_diagnostic (const contract_proof_query &query, bool always_false,
			const vec<contract_proof_cause> &causes)
{
  auto_vec<contract_shadow_note> notes;
  for (const contract_proof_cause &c : causes)
    {
      contract_shadow_note n = { c.loc, c.always_false };
      notes.safe_push (n);
    }
  /* An override pair's, at any optimization level and whether or not the
     shadows prove: issued only if its function is compiled (DECISIONS.md
     E150), which is known at the end.  */
  if (query.override_pair && !flag_syntax_only && !pch_file)
    {
      contract_shadow_defer_diagnostic (current_function_decl, query.loc,
					query.pred_loc,
					query.semantic == CES_PROVE,
					always_false, shadow_context (),
					query.override_pair, notes);
      return true;
    }
  if (!shadow_defers_diagnostics_p ())
    return false;
  contract_shadow_defer_diagnostic (current_function_decl, query.loc,
				    query.pred_loc,
				    query.semantic == CES_PROVE, always_false,
				    shadow_context (), 0, notes);
  return true;
}

/* Record QUERY, decided now with the result R, as nested in the obligation
   whose handle (its id plus one) is QUERY.nest's (E169).  */

static unsigned
shadow_nest_decided (const contract_proof_query &query,
		     contract_proof_result r)
{
  contract_shadow_result sr;
  switch (r)
    {
    case CPR_SOUND: sr = CSR_SOUND; break;
    case CPR_CONDITIONAL: sr = CSR_CONDITIONAL; break;
    case CPR_ALWAYS_FALSE: sr = CSR_ALWAYS_FALSE; break;
    default: sr = CSR_UNPROVEN; break;
    }
  return 1 + contract_shadow_record_nested (current_function_decl, query.loc,
					    query.pred_loc,
					    query.semantic == CES_PROVE, sr,
					    shadow_context (),
					    query.nest->handle,
					    query.override_pair);
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
  shadow_note_premise,
  shadow_nest_decided,
  /*nested_predicate=*/NULL
};

/* walk_tree callback: find a .CONTRACT_PROVEN call.  */

static tree
find_contract_proven_r (tree *tp, int *, void *)
{
  if (TREE_CODE (*tp) == CALL_EXPR
      && CALL_EXPR_FN (*tp) == NULL_TREE
      && CALL_EXPR_IFN (*tp) == IFN_CONTRACT_PROVEN)
    return *tp;
  return NULL_TREE;
}

/* True if COND is a region's guard, .CONTRACT_SHADOW_GUARD (...).  */

static bool
region_guard_p (tree cond)
{
  return (TREE_CODE (cond) == CALL_EXPR
	  && CALL_EXPR_FN (cond) == NULL_TREE
	  && CALL_EXPR_IFN (cond) == IFN_CONTRACT_SHADOW_GUARD);
}

/* True if the statement T does nothing in the real function: it is empty,
   or made of regions, of the assumptions made of obligations if
   gimplification drops them (UNOPTIMIZED, as it drops every [[assume]]
   whose condition is not simple at -O0), and of `if' statements whose
   conditions have no side effects and whose arms are such statements.  */

static bool
region_only_p (tree t, bool unoptimized)
{
  if (!t)
    return true;
  switch (TREE_CODE (t))
    {
    case NOP_EXPR:
      return VOID_TYPE_P (TREE_TYPE (t)) && integer_zerop (TREE_OPERAND (t, 0));
    case STATEMENT_LIST:
      for (tree_stmt_iterator i = tsi_start (t); !tsi_end_p (i); tsi_next (&i))
	if (!region_only_p (tsi_stmt (i), unoptimized))
	  return false;
      return true;
    case BIND_EXPR:
      return (!BIND_EXPR_VARS (t)
	      && region_only_p (BIND_EXPR_BODY (t), unoptimized));
    case COMPOUND_EXPR:
      return (region_only_p (TREE_OPERAND (t, 0), unoptimized)
	      && region_only_p (TREE_OPERAND (t, 1), unoptimized));
    case COND_EXPR:
      if (region_guard_p (COND_EXPR_COND (t)))
	return true;
      return (VOID_TYPE_P (TREE_TYPE (t))
	      && !TREE_SIDE_EFFECTS (COND_EXPR_COND (t))
	      && region_only_p (COND_EXPR_THEN (t), unoptimized)
	      && region_only_p (COND_EXPR_ELSE (t), unoptimized));
    case CALL_EXPR:
      return (unoptimized
	      && CALL_EXPR_FN (t) == NULL_TREE
	      && CALL_EXPR_IFN (t) == IFN_ASSUME
	      && walk_tree_without_duplicates (&CALL_EXPR_ARG (t, 0),
					       find_contract_proven_r, NULL));
    default:
      return false;
    }
}

/* walk_tree callback: find what a copy of a condition must not duplicate
   -- a declaration, a label, a temporary, a statement.  */

static tree
uncopyable_r (tree *tp, int *walk_subtrees, void *)
{
  switch (TREE_CODE (*tp))
    {
    case BIND_EXPR:
    case DECL_EXPR:
    case LABEL_EXPR:
    case TARGET_EXPR:
    case STATEMENT_LIST:
    case SAVE_EXPR:
      return *tp;
    default:
      if (TYPE_P (*tp) || DECL_P (*tp))
	*walk_subtrees = 0;
      return NULL_TREE;
    }
}

/* The statement T, region_only_p, as the shadows run it: its regions, and
   copies of its conditions, without its scopes, which stay with T.  */

static tree
shadow_copy (tree t)
{
  if (!t)
    return t;
  switch (TREE_CODE (t))
    {
    case STATEMENT_LIST:
      {
	tree list = NULL_TREE;
	for (tree_stmt_iterator i = tsi_start (t); !tsi_end_p (i);
	     tsi_next (&i))
	  append_to_statement_list_force (shadow_copy (tsi_stmt (i)), &list);
	return list ? list : build_empty_stmt (UNKNOWN_LOCATION);
      }
    case BIND_EXPR:
      return shadow_copy (BIND_EXPR_BODY (t));
    case COMPOUND_EXPR:
      return build2 (COMPOUND_EXPR, void_type_node,
		     shadow_copy (TREE_OPERAND (t, 0)),
		     shadow_copy (TREE_OPERAND (t, 1)));
    case COND_EXPR:
      if (region_guard_p (COND_EXPR_COND (t)))
	return t;
      return build3_loc (EXPR_LOCATION (t), COND_EXPR, void_type_node,
			 unshare_expr (COND_EXPR_COND (t)),
			 shadow_copy (COND_EXPR_THEN (t)),
			 shadow_copy (COND_EXPR_ELSE (t)));
    default:
      /* An empty statement, or an assumption made of an obligation.  */
      return t;
    }
}

/* Replace each region in the statement T, region_only_p, and each
   assumption made of an obligation, by an empty statement, in place.
   Returns T, or what replaces it.  */

static tree
strip_regions (tree t)
{
  if (!t)
    return t;
  switch (TREE_CODE (t))
    {
    case STATEMENT_LIST:
      TREE_SIDE_EFFECTS (t) = 0;
      for (tree_stmt_iterator i = tsi_start (t); !tsi_end_p (i);
	   tsi_next (&i))
	{
	  *tsi_stmt_ptr (i) = strip_regions (tsi_stmt (i));
	  TREE_SIDE_EFFECTS (t) |= TREE_SIDE_EFFECTS (tsi_stmt (i));
	}
      return t;
    case BIND_EXPR:
      /* The front end makes every scope a statement with side effects.  */
      BIND_EXPR_BODY (t) = strip_regions (BIND_EXPR_BODY (t));
      return t;
    case COMPOUND_EXPR:
      TREE_OPERAND (t, 0) = strip_regions (TREE_OPERAND (t, 0));
      TREE_OPERAND (t, 1) = strip_regions (TREE_OPERAND (t, 1));
      TREE_SIDE_EFFECTS (t) = (TREE_SIDE_EFFECTS (TREE_OPERAND (t, 0))
			       || TREE_SIDE_EFFECTS (TREE_OPERAND (t, 1)));
      return t;
    case COND_EXPR:
      if (region_guard_p (COND_EXPR_COND (t)))
	return build_empty_stmt (EXPR_LOCATION (t));
      COND_EXPR_THEN (t) = strip_regions (COND_EXPR_THEN (t));
      COND_EXPR_ELSE (t) = strip_regions (COND_EXPR_ELSE (t));
      /* The C front end leaves out an `else' arm.  */
      TREE_SIDE_EFFECTS (t)
	= (TREE_SIDE_EFFECTS (COND_EXPR_COND (t))
	   || (COND_EXPR_THEN (t) && TREE_SIDE_EFFECTS (COND_EXPR_THEN (t)))
	   || (COND_EXPR_ELSE (t) && TREE_SIDE_EFFECTS (COND_EXPR_ELSE (t))));
      return t;
    case CALL_EXPR:
      return build_empty_stmt (EXPR_LOCATION (t));
    default:
      return t;
    }
}

/* walk_tree callback for c_contract_hoist_region_guards; DATA points to
   whether the function is unoptimized.  */

static tree
hoist_region_guards_r (tree *tp, int *walk_subtrees, void *data)
{
  tree t = *tp;
  if (TYPE_P (t) || DECL_P (t))
    {
      *walk_subtrees = 0;
      return NULL_TREE;
    }
  if (TREE_CODE (t) != COND_EXPR || !VOID_TYPE_P (TREE_TYPE (t)))
    return NULL_TREE;
  /* Nothing in a region is real code.  */
  if (region_guard_p (COND_EXPR_COND (t)))
    {
      *walk_subtrees = 0;
      return NULL_TREE;
    }
  bool unoptimized = *(bool *) data;
  if (TREE_SIDE_EFFECTS (t)
      && !TREE_SIDE_EFFECTS (COND_EXPR_COND (t))
      && region_only_p (COND_EXPR_THEN (t), unoptimized)
      && region_only_p (COND_EXPR_ELSE (t), unoptimized)
      && !walk_tree_without_duplicates (&COND_EXPR_COND (t), uncopyable_r,
					NULL))
    {
      location_t loc = EXPR_LOCATION (t);
      tree guard
	= build_call_expr_internal_loc (loc, IFN_CONTRACT_SHADOW_GUARD,
					boolean_type_node, 1,
					build_int_cst (unsigned_type_node, 0));
      contract_shadow_note_region ();
      tree shadow = shadow_copy (t);
      tree real = strip_regions (t);
      *tp = build3_loc (loc, COND_EXPR, void_type_node, guard, shadow, real);
      *walk_subtrees = 0;
    }
  return NULL_TREE;
}

/* D4405's proof optimization level: an `if' statement whose condition
   has no side effects and whose arms hold nothing but regions is a
   statement whose code depends on the regions: the gimplifier lowers it
   differently with them, and -O0 emits what is left once they are gone
   (a test of the condition, without anything to do).  So that the real
   function's code is the one it has without the regions, make the
   statement an ordinary region of its own, executed in every shadow,
   which runs it as written, and give the real function the statement
   without its regions: `if (c) R' becomes
   `if (.CONTRACT_SHADOW_GUARD (0)) if (c) R; else if (c);'.  For FNDECL,
   whose body is generic.  */

void
c_contract_hoist_region_guards (tree fndecl)
{
  if (!(flag_contracts_prove_shadow || flag_contracts_prove_override)
      || !contract_shadow_level_p ()
      || !DECL_SAVED_TREE (fndecl))
    return;
  bool unoptimized = !opt_for_fn (fndecl, optimize);
  walk_tree_without_duplicates (&DECL_SAVED_TREE (fndecl),
				hoist_region_guards_r, &unoptimized);
}
