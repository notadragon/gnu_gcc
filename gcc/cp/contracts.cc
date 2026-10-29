/* C++ contracts.

   Copyright (C) 2020-2026 Free Software Foundation, Inc.
   Originally by Jeff Chapman II (jchapman@lock3software.com) for proposed
   C++20 contracts.
   Rewritten for C++26 contracts by:
     Nina Ranns (dinka.ranns@googlemail.com)
     Iain Sandoe (iain@sandoe.co.uk)
     Ville Voutilainen (ville.voutilainen@gmail.com).

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

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "cp-tree.h"
#include "stringpool.h"
#include "diagnostic.h"
#include "options.h"
#include "contracts.h"
#include "cp/contracts-config.h"
#include "cp-objcp-common.h"
#include "c-family/contracts-prove.h"
#include "tree.h"
#include "tree-inline.h"
#include "attribs.h"
#include "tree-iterator.h"
#include "print-tree.h"
#include "stor-layout.h"
#include "intl.h"
#include "cgraph.h"
#include "contracts-shadow.h"
#include "opts.h"
#include "output.h"
#include "varasm.h"
#include "ubsan.h"
#include "predict.h"

/*  Design notes.

  There are three phases:
    1. Parsing and semantic checks.
       Most of the code for this is in the parser, with helpers provided here.
    2. Emitting contract assertion AST nodes into function bodies.
       This is initiated from "finish_function ()"
    3. Lowering the contract assertion AST nodes to control flow, constant
       data and calls to the violation handler.
       This is initiated from "cp_genericize ()".

  The organisation of the code in this file is intended to follow those three
  phases where possible.

  Contract Assertion State
  ========================

  contract_assert () does not require any special handling and can be
  represented directly by AST inserted in the function body.

  'pre' and 'post' function contract specifiers require most of the special
  handling, since they must be tracked across re-declarations of functions and
  there are constraints on how such specifiers may change in these cases.

  The contracts specification identifies a "first declaration" of any given
  function - which is the first encountered when parsing a given TU.
  Subsequent re-declarations may not add or change the function contract
  specifiers from any introduced on this first declaration.  It is, however,
  permitted to omit specifiers on re-declarations.

  Since the implementation of GCC's (re-)declarations is a destructive merge
  we need to keep some state on the side to determine whether the re-declaration
  rules are met.  In this current design we have chosen not to add another tree
  to each function decl but, instead, keep a map from function decl to contract
  specifier state.  In this state we record the 'first declaration' specifiers
  which are used to validate re-declaration(s) and to report the initial state
  in diagnostics.

  We need (for example) to compare
    pre ( x > 2 ) equal to
    pre ( z > 2 ) when x and z refer to the same function parameter in a
    re-declaration.

  The mechanism used to determine if two contracts are the same is to compare
  the folded trees.  This makes use of current compiler machinery, rather than
  constructing some new AST comparison scheme.  However, it does introduce an
  additional complexity in that we need to defer such comparison until parsing
  is complete - and function contract specifiers in class declarations must be
  deferred parses, since it is also permitted for specifiers to refer to class
  members.

  When we encounter a definition, the parameter names in a function decl are
  re-written to match those of the definition (thus the expected names will
  appear in debug information etc).  At this point, we also need to re-map
  any function parameter names that appear in function contract specifiers
  to agree with those of the definition - although we intend to keep the
  'first declaration' record consistent for diagnostics.

  Since we shared some code from the C++2a contracts implementation, pre and
  post specifiers are represented by chains of attributes, where the payload
  of the attribute is an AST node.  However during the parse, these are not
  inserted into the function bodies, but kept in the decl-keyed state described
  above.  A future improvement planned here is to store the specifiers using a
  tree vec instead of the attribute list.

  Emitting contract AST
  =====================

  When we reach `finish_function ()` and therefore are committed to potentially
  emitting code for an instance, we build a new variant of the function body
  with the pre-condition AST inserted before the user's function body, and the
  post condition AST (if any) linked into the function return.

  Lowering the contract assertion AST
  ===================================

  In all cases (pre, post, contract_assert) the AST node is lowered to control
  flow and (potentially) calls to the violation handler and/or termination.
  This is done during `cp_genericize ()`.  The control flow depends on the
  contract's evaluation semantic, resolved for each contract from the P3595
  configuration and the command line; the semantics behave as follows:

    'ignore'	    : contract assertion AST is lowered to 'nothing',
		      i.e. omitted.
    'enforce'	    : contract assertion AST is lowered to a check, if this
		      fails a violation handler is called, followed by
		      std::terminate().
    'quick_enforce' : contract assertion AST is lowered to a check, if this
		      fails, std::terminate () is called.
    'observe'	    : contract assertion AST is lowered to a check, if this
		      fails, a violation handler is called, the code then
		      continues.

  In each case, the "check" might be a simple 'if' (when it is determined that
  the assertion condition does not throw) or the condition evaluation will be
  wrapped in a try-catch block that treats any exception thrown when evaluating
  the check as equivalent to a failed check.  It is noted in the violation data
  object whether a check failed because of an exception raised in evaluation.

  At present, a simple (but potentially space-inefficient) scheme is used to
  store constant data objects that represent the read-only data for the
  violation.  The exact form of this is subject to revision as it represents
  ABI that must be agreed between implementations (as of this point, that
  discussion is not yet concluded).  */

/* Contract matching.  */

bool comparing_contracts;

/* True if the contract is valid.  */

static bool
contract_valid_p (tree contract)
{
  return CONTRACT_CONDITION (contract) != error_mark_node;
}

/* True if every contract in CONTRACTS parsed.  Only meaningful once they
   have been: a DEFERRED_PARSE condition is not error_mark_node, so an
   unparsed contract reads as valid here.  */

static bool
contract_all_valid_p (tree contracts)
{
  if (!contracts)
    return true;

  for (tree contract : tree_vec_range (contracts))
    if (!contract_valid_p (contract))
      return false;
  return true;
}

static tree postcondition_capture_initializer (tree);

/* True if A and B, parts of two contracts, are the same as written: the
   one-definition rule compares token sequences, so two that fold to the same
   value need not match.  */

static bool
contract_parts_equal_p (tree a, tree b)
{
  bool saved_comparing_contracts = comparing_contracts;
  comparing_contracts = true;
  bool equal_p = cp_tree_equal (a, b);
  comparing_contracts = saved_comparing_contracts;
  return equal_p;
}

/* Diagnose NEW_CONTRACT as not matching OLD_CONTRACT, GMSGID saying in what.
   Returns true, mismatched_contracts_p's answer.  */

static bool
diagnose_contract_mismatch (tree old_contract, tree new_contract,
			    const char *gmsgid)
{
  auto_diagnostic_group d;
  error_at (EXPR_LOCATION (new_contract), gmsgid);
  inform (EXPR_LOCATION (old_contract), "previous contract here");
  return true;
}

/* Compare the contract conditions of OLD_CONTRACT and NEW_CONTRACT.
   Returns false if the conditions are equivalent, and true otherwise.  */

static bool
mismatched_contracts_p (tree old_contract, tree new_contract)
{
  /* Different kinds of contracts do not match.  */
  if (TREE_CODE (old_contract) != TREE_CODE (new_contract))
    {
      auto_diagnostic_group d;
      error_at (EXPR_LOCATION (new_contract),
		"mismatched contract specifier in declaration");
      inform (EXPR_LOCATION (old_contract), "previous contract here");
      return true;
    }

  /* A deferred contract tentatively matches.  */
  if (CONTRACT_CONDITION_DEFERRED_P (new_contract))
    return false;

  /* A postcondition's result-name-introducer is part of the token sequence
     the one-definition rule compares: both have one, or neither does.  */
  if (POSTCONDITION_P (old_contract)
      && ((POSTCONDITION_IDENTIFIER (old_contract) == NULL_TREE)
	  != (POSTCONDITION_IDENTIFIER (new_contract) == NULL_TREE)))
    {
      auto_diagnostic_group d;
      error_at (EXPR_LOCATION (new_contract),
		"mismatched postcondition result name in declaration");
      inform (EXPR_LOCATION (old_contract), "previous contract here");
      return true;
    }

  /* A condition already diagnosed as erroneous is not compared again
     (GCC-938).  */
  if (CONTRACT_CONDITION (old_contract) == error_mark_node
      || CONTRACT_CONDITION (new_contract) == error_mark_node)
    return false;

  /* Compare the conditions of the contracts as written, not folded: the
     one-definition rule compares token sequences, and two conditions that
     fold to the same value need not be the same.  */
  bool saved_comparing_contracts = comparing_contracts;
  comparing_contracts = true;
  bool matching_p = cp_tree_equal (CONTRACT_CONDITION (old_contract),
				   CONTRACT_CONDITION (new_contract));
  comparing_contracts = saved_comparing_contracts;

  if (!matching_p)
    {
      auto_diagnostic_group d;
      error_at (EXPR_LOCATION (CONTRACT_CONDITION (new_contract)),
		"mismatched contract condition in declaration");
      inform (EXPR_LOCATION (CONTRACT_CONDITION (old_contract)),
	      "previous contract here");
      return true;
    }

  /* Compare user-defined diagnostic messages (P3099).  Both must specify
     a message with the same text, or neither specifies one.  */
  tree old_msg = CONTRACT_MESSAGE (old_contract);
  tree new_msg = CONTRACT_MESSAGE (new_contract);
  bool msg_strings = (old_msg && new_msg
		      && TREE_CODE (old_msg) == STRING_CST
		      && TREE_CODE (new_msg) == STRING_CST);
  if ((old_msg == NULL_TREE) != (new_msg == NULL_TREE)
      || (msg_strings
	  && (TREE_STRING_LENGTH (old_msg) != TREE_STRING_LENGTH (new_msg)
	      || memcmp (TREE_STRING_POINTER (old_msg),
			 TREE_STRING_POINTER (new_msg),
			 TREE_STRING_LENGTH (old_msg)) != 0))
      /* A message still as written in a template.  */
      || (old_msg && new_msg && !msg_strings
	  && !cp_tree_equal (old_msg, new_msg)))
    return diagnose_contract_mismatch
      (old_contract, new_contract,
       G_("mismatched contract diagnostic message in declaration"));

  /* Compare assertion-control labels (P3400).  Both must specify the same
     label, or neither specifies one.  */
  tree old_label = CONTRACT_LABEL (old_contract);
  tree new_label = CONTRACT_LABEL (new_contract);
  /* As written, as for the condition: two distinct objects of equal value
     fold to equal constructors.  */
  if ((old_label == NULL_TREE) != (new_label == NULL_TREE)
      || (old_label && !contract_parts_equal_p (old_label, new_label)))
    return diagnose_contract_mismatch
      (old_contract, new_contract,
       G_("mismatched assertion-control label in declaration"));

  /* Compare postcondition captures (P3098).  */
  if (POSTCONDITION_P (old_contract))
    {
      tree old_caps = POSTCONDITION_CAPTURES (old_contract);
      tree new_caps = POSTCONDITION_CAPTURES (new_contract);

      /* Both have captures or neither, pairwise the same: same count,
	 same names, same initializers.  */
      tree oc = old_caps, nc = new_caps;
      for (; oc && nc; oc = TREE_CHAIN (oc), nc = TREE_CHAIN (nc))
	{
	  tree old_var = TREE_VALUE (oc);
	  tree new_var = TREE_VALUE (nc);
	  if (DECL_NAME (old_var) != DECL_NAME (new_var)
	      || !contract_parts_equal_p
		    (postcondition_capture_initializer (old_var),
		     postcondition_capture_initializer (new_var)))
	    break;
	}
      if (oc || nc)
	return diagnose_contract_mismatch
	  (old_contract, new_contract,
	   G_("mismatched postcondition captures in declaration"));
    }

  /* Compare requires-clauses (P4283).  Both must specify the same
     constraint, or neither specifies one.  */
  tree old_req = CONTRACT_REQUIRES_CLAUSE (old_contract);
  tree new_req = CONTRACT_REQUIRES_CLAUSE (new_contract);
  if ((old_req == NULL_TREE) != (new_req == NULL_TREE)
      || (old_req && !contract_parts_equal_p (old_req, new_req)))
    return diagnose_contract_mismatch
      (old_contract, new_contract,
       G_("mismatched requires clause on contract assertion in declaration"));

  return false;
}

/* Compare the contract specifiers of OLDDECL and NEWDECL. Returns true
   if the contracts match, and false if they differ.  */

static bool
match_contract_specifiers (location_t oldloc, tree old_contracts,
			   location_t newloc, tree new_contracts)
{
  /* Contracts only match if they are both specified.  */
  if (!old_contracts || !new_contracts)
    return true;

  int old_len = TREE_VEC_LENGTH (old_contracts);
  int new_len = TREE_VEC_LENGTH (new_contracts);

  /* If we don't have the same number, the contracts don't match.  */
  if (old_len != new_len)
    {
      auto_diagnostic_group d;
      error_at (newloc,
		"declaration has a different number of contracts than "
		"previously declared");
      inform (oldloc,
	      new_len > old_len
	      ? "previous declaration with fewer contracts here"
	      : "previous declaration with more contracts here");
      return false;
    }

  /* Compare each contract in turn.  */
  for (int ix = 0; ix < MIN (old_len, new_len); ix++)
    {
      tree old_contract = TREE_VEC_ELT (old_contracts, ix);
      tree new_contract = TREE_VEC_ELT (new_contracts, ix);

      /* If either contract is ill-formed, skip the rest of the comparison,
	 since we've already diagnosed an error.  */
      if (!contract_valid_p (new_contract) || !contract_valid_p (old_contract))
	return false;

      if (mismatched_contracts_p (old_contract, new_contract))
	return false;
    }


  return true;
}

/* True if SEM emits no check of the predicate: ignore, and the P3100
   "assume" semantic, which never evaluates it either.  */

static inline bool
contract_semantic_emits_no_check (uint16_t sem)
{
  return sem == CES_IGNORE || sem == CES_ASSUME;
}

/* The values a P3595 dynamic selector may return: every evaluation
   semantic.  One that is not allowed for the contract -- assume without
   -fcontracts-allow-assume, a noexcept one without -fcontracts-p4298, prove
   or diagnose without -fcontracts-p4405, or one a label excludes -- is
   adjusted to the allowed set (D3595R1, "Adjusting a Semantic to the Allowed
   Set"); any other value is an invalid selection, reported as an enforced
   violation.  */

static auto_vec<int>
dynamic_raw_semantics ()
{
  auto_vec<int> v;
  for (int s = CES_IGNORE; s <= CES_LAST; s++)
    v.safe_push (s);
  return v;
}

/* The semantics a dynamically selected check dispatches on once the
   selector's value is adjusted: the four C++26 semantics, and under
   -fcontracts-p4298 the two noexcept ones as well.  An allowed assume, prove
   or diagnose takes the ignore arm.  */

static auto_vec<int>
dynamic_dispatch_semantics ()
{
  auto_vec<int> v;
  for (int s = CES_IGNORE; s <= CES_QUICK; s++)
    v.safe_push (s);
  if (flag_contracts_p4298)
    {
      v.safe_push (CES_NOEXCEPT_OBSERVE);
      v.safe_push (CES_NOEXCEPT_ENFORCE);
    }
  return v;
}

/* True if SEM never evaluates the predicate at run time: the semantics of
   contract_semantic_emits_no_check, and the P4405 prove and diagnose.  Where
   nothing would be diagnosed -- constant evaluation, the initialization of
   postcondition captures, the semantic a dynamic selector returns -- prove
   and diagnose behave exactly as ignore.  */

static inline bool
contract_semantic_has_no_runtime_check (uint16_t sem)
{
  return (contract_semantic_emits_no_check (sem)
	  || contract_semantic_proving_p ((contract_evaluation_semantic) sem));
}

/* Diagnose the analysis evaluation of CONTRACT with the P4405 semantic SEM,
   prove or diagnose, unless its predicate is proven: an error for prove, a
   -Wcontract-unproven warning for diagnose.  LOC is where the evaluation
   happens -- the contract itself for one in the function body, the call for
   one at a call -- and the predicate is noted when it is elsewhere.  For
   one at a call, CALL is the call to FNDECL.  The predicate is never
   evaluated.  The proof itself is the proving strategies'
   (c-family/contracts-prove.h); this only describes the evaluation.

   PRED, if given, is the predicate as it would be evaluated there, in place
   of the contract's own.  MARKER is non-null if the caller can place a
   marker where the evaluation happens; if a strategy then defers the
   evaluation, *MARKER is set to it (for a caller-side evaluation, built for
   the predicate with the callee's parameters replaced by the call's
   arguments).  OPAQUE is true if the predicate cannot be described
   (contract_for_caller_side_proof): it is then not proven.  OVERRIDE_PAIR
   is 0, or the override pair for which the evaluation is made
   (begin_override_shadows), in its override shadow if OVERRIDE_SHADOW and
   otherwise as an obligation of the overriding function itself.  Returns
   the result.  */

static contract_proof_result
diagnose_unproven_contract (tree contract, contract_evaluation_semantic sem,
			    location_t loc, tree fndecl = NULL_TREE,
			    tree call = NULL_TREE, tree *marker = NULL,
			    tree pred = NULL_TREE, bool opaque = false,
			    unsigned override_pair = 0,
			    bool override_shadow = true)
{
  gcc_checking_assert (contract_semantic_proving_p (sem));
  contract_proof_query query = {};
  query.predicate = pred ? pred : CONTRACT_CONDITION (contract);
  query.semantic = sem;
  query.kind = (PRECONDITION_P (contract) ? CAK_PRE
		: POSTCONDITION_P (contract) ? CAK_POST : CAK_ASSERT);
  query.loc = loc;
  query.pred_loc = cp_expr_loc_or_loc (CONTRACT_CONDITION (contract),
				       EXPR_LOCATION (contract));
  /* A precondition sees the parameters as the call initialized them, so the
     strategies may use the arguments.  The predicate names the parameters
     of FNDECL's origin, `this' included, which the call passes in order --
     unless an in-charge or VTT parameter gets in the way.  In the callee
     (a constructor or destructor clone) it does; in its origin only, the
     predicate is made to name the clone's own parameters instead
     (GCC-917): a call of the complete object constructor of a class with a
     virtual base calls a clone that has neither.  */
  tree origin = fndecl ? DECL_ORIGIN (fndecl) : NULL_TREE;
  if (call && PRECONDITION_P (contract)
      && TREE_CODE (call) == CALL_EXPR
      && !DECL_HAS_IN_CHARGE_PARM_P (fndecl)
      && !DECL_HAS_VTT_PARM_P (fndecl))
    {
      if (!DECL_HAS_IN_CHARGE_PARM_P (origin)
	  && !DECL_HAS_VTT_PARM_P (origin))
	{
	  query.call = call;
	  query.parms = DECL_ARGUMENTS (origin);
	}
      else if (fndecl != origin)
	{
	  /* Map ORIGIN's parameters, but for the in-charge and VTT ones, to
	     FNDECL's, which must line up.  */
	  hash_map<tree, tree> map;
	  tree c = DECL_ARGUMENTS (fndecl);
	  bool lined_up = true;
	  for (tree o = DECL_ARGUMENTS (origin); o; o = DECL_CHAIN (o))
	    if (DECL_NAME (o) != in_charge_identifier
		&& DECL_NAME (o) != vtt_parm_identifier)
	      {
		if (!c)
		  lined_up = false;
		else
		  {
		    map.put (o, c);
		    c = DECL_CHAIN (c);
		  }
	      }
	  if (lined_up && !c)
	    {
	      query.predicate = unshare_expr (query.predicate);
	      cp_walk_tree (&query.predicate,
			    [] (tree *tp, int *walk_subtrees, void *data)
			    {
			      if (TYPE_P (*tp))
				*walk_subtrees = 0;
			      else if (TREE_CODE (*tp) == PARM_DECL)
				if (tree *r = (static_cast<hash_map<tree, tree> *>
					       (data)->get (*tp)))
				  *tp = *r;
			      return NULL_TREE;
			    }, &map, NULL);
	      query.call = call;
	      query.parms = DECL_ARGUMENTS (fndecl);
	    }
	}
    }
  query.opaque = opaque;
  query.override_pair = override_pair;
  query.override_shadow = override_pair && override_shadow;
  /* A caller-side marker needs the arguments.  */
  query.deferrable = marker && (!call || query.call) && !opaque;
  /* P4405 proving hook.  */
  return contract_diagnose_unless_proven (query,
					  query.deferrable ? marker : NULL);
}

static void remap_dummy_this (tree, tree *);
static void remap_retval (tree, tree);
static tree compute_caller_semantic_tuple (tree, location_t, tree,
					  vec<contract_evaluation_semantic> *);
static bool handle_caller_contracts_p (tree);

/* True if a proving strategy may defer the evaluations in FN's body
   (contract_proof_defers_p), which is not the case for a module's
   functions, whose bodies may be compiled by another translation unit than
   the one that placed the markers, nor for a coroutine, whose body moves
   into helper functions.  */

static bool
contract_proof_deferral_ok_p (tree fn)
{
  return (contract_proof_defers_p (fn)
	  && !modules_p ()
	  && !(DECL_LANG_SPECIFIC (fn) && DECL_COROUTINE_P (fn))
	  && !DECL_RAMP_FN (fn));
}

/* True if a proving strategy may defer CONTRACT's evaluation here, or
   assume it, in the current function.  A postcondition with captures may
   not: the captures are initialized only for a checked evaluation.  */

static bool
contract_proof_deferrable_p (tree contract)
{
  return (contract_proof_deferral_ok_p (current_function_decl)
	  && !(POSTCONDITION_P (contract)
	       && POSTCONDITION_CAPTURES (contract)));
}

/* CONTRACT's predicate as a check of it would evaluate it here, or
   NULL_TREE on error.  */

static tree
contract_check_condition (tree contract)
{
  remap_dummy_this (current_function_decl, &CONTRACT_CONDITION (contract));
  if (POSTCONDITION_P (contract) && !flag_contract_checks_outlined
      && CONTRACT_CONDITION (contract) != error_mark_node)
    remap_retval (current_function_decl, contract);
  tree cond = CONTRACT_CONDITION (contract);
  return cond == error_mark_node ? NULL_TREE : cond;
}

/* CHECK, the check of CONTRACT under SEMANTIC where a strategy may defer
   the evaluation (contract_proof_deferrable_p), followed by what lets the
   strategies that defer evaluations assume the predicate, when one wants
   that (contract_proof_assumption).  */

static tree
add_contract_proof_assumption (tree contract,
			       contract_evaluation_semantic semantic,
			       tree check)
{
  if (!check || check == error_mark_node
      || contract_semantic_proving_p (semantic))
    return check;
  tree pred = contract_check_condition (contract);
  tree assumption = contract_proof_assumption (pred, semantic,
					       EXPR_LOCATION (contract));
  if (!assumption)
    return check;
  if (check == void_node)
    return assumption;
  tree list = alloc_stmt_list ();
  append_to_statement_list_force (check, &list);
  append_to_statement_list_force (assumption, &list);
  return list;
}

/* True if SEM never permits an exception to escape a handler call: the
   contract is either unchecked, or checked through a terminating entry
   point.  Used to decide whether a compiler-synthesized wrapper function
   (P3097, P3098) can be marked noexcept.  */

static inline bool
contract_semantic_is_nonthrowing (uint16_t sem)
{
  return sem == CES_IGNORE || sem == CES_QUICK || sem == CES_ASSUME
	 || sem == CES_NOEXCEPT_ENFORCE || sem == CES_NOEXCEPT_OBSERVE
	 || sem == CES_PROVE || sem == CES_DIAGNOSE;
}

/* True if every contract in CONTRACTS -- a TREE_VEC from
   get_fn_contract_specifiers -- has a statically fixed, nonthrowing
   evaluation semantic: no P3595 dynamic descriptor (CONTRACT_DYNAMIC) and no
   assertion-control label (CONTRACT_LABEL) that could select a throwing
   semantic at run time.  FNDECL is the context for
   ensure_evaluation_semantic.

   If ONLY_KIND is PRECONDITION_STMT or POSTCONDITION_STMT, contracts of the
   other kind are skipped: the outlined pre and post functions each check one
   kind.  ERROR_MARK means every contract.

   The semantic is resolved before CONTRACT_DYNAMIC is read, because resolving
   it is what fills that field in.  Returns false for anything it cannot
   prove.  */

static bool
all_contracts_statically_nonthrowing (tree contracts, tree fndecl,
				      tree_code only_kind = ERROR_MARK)
{
  if (!contracts)
    return true;

  for (tree contract : tree_vec_range (contracts))
    {
      /* The .pre function also runs the postconditions' capture
	 initializers (P3098), which can throw into the handler.  */
      if (only_kind == PRECONDITION_STMT
	  && TREE_CODE (contract) == POSTCONDITION_STMT
	  && POSTCONDITION_CAPTURES (contract))
	;
      else if (only_kind != ERROR_MARK && TREE_CODE (contract) != only_kind)
	continue;
      contract_evaluation_semantic sem
	= ensure_evaluation_semantic (contract, fndecl, /*in_ce=*/false);
      if (CONTRACT_DYNAMIC (contract) || CONTRACT_LABEL (contract))
	return false;
      if (!contract_semantic_is_nonthrowing (sem))
	return false;
    }
  return true;
}

/* True if an evaluation of CONTRACT with the semantic SEM is lowered as an
   optimizer assumption (build_contract_assumption): one with the P3100
   assume semantic, unless CONTRACT is a postcondition with captures, which
   are initialized only for an evaluation that checks the predicate.  Such
   an evaluation needs the contract in the function body, as a check
   does.  */

static bool
contract_lowered_as_assumption_p (tree contract, uint16_t sem)
{
  return (sem == CES_ASSUME
	  && !(POSTCONDITION_P (contract) && POSTCONDITION_CAPTURES (contract)));
}

/* Return true if CONTRACT is checked or assumed under the current build
   configuration.  Returns true if either the runtime or the constexpr
   evaluation semantic is non-ignore, since either can require the
   contract to be present in the function body.  D4405: so does the
   analysis evaluation of a postcondition with the prove or diagnose
   semantic, which is diagnosed in the body (build_analysis_evaluation);
   that of a precondition is at each call instead.  */

static bool
contract_active_p (tree contract, tree fndecl)
{
  /* Resolve the runtime semantic first; this also caches the P3595
     dynamic-selector descriptor (if any) on the contract node.  */
  contract_evaluation_semantic runtime;
  contract_evaluation_semantic analysis
    = ensure_analysis_semantic (contract, fndecl, &runtime);
  bool runtime_active = !contract_semantic_emits_no_check (runtime);
  if (!PRECONDITION_P (contract) && contract_semantic_proving_p (analysis))
    runtime_active = true;
  /* P3100: an assume evaluation is an optimizer assumption in the body.  */
  if (contract_lowered_as_assumption_p
	(contract, ensure_evaluation_semantic (contract, fndecl, false)))
    runtime_active = true;

  /* A dynamic contract is always active at run time regardless of its
     compile-time default: the selector may return a checking semantic even
     when the default is "ignore" (P3595).  */
  if (CONTRACT_DYNAMIC (contract))
    runtime_active = true;

  return runtime_active
    || !contract_semantic_has_no_runtime_check
	   (ensure_evaluation_semantic (contract, fndecl, true));
}

/* P4405: the constructor FN inherits through one or more inheriting
   constructors, if FN is an inheriting constructor (or a clone of one);
   otherwise NULL_TREE.  For a specialization of an inheriting constructor
   template that is the inherited template's specialization; the template
   itself leads to a TEMPLATE_DECL, and gives NULL_TREE.  */

static tree
inherited_ctor_for_proving (tree fn)
{
  if (!flag_new_inheriting_ctors || !DECL_CONSTRUCTOR_P (fn))
    return NULL_TREE;
  tree orig = DECL_ORIGIN (fn);
  tree inh = orig;
  while (tree i = DECL_INHERITED_CTOR (inh))
    inh = OVL_FIRST (i);
  if (inh == orig || TREE_CODE (inh) != FUNCTION_DECL)
    return NULL_TREE;
  return inh;
}

static contract_query make_contract_query (tree, tree);

/* D4405: make the contracts of CALLEE ready for resolving the semantics of
   their analysis evaluations at a call of it (call_analysis_semantic).  A
   specialization's contracts are the pattern's until its definition is
   instantiated (contract_for_caller_side_proof); with a label that is not
   dependent, the analysis semantic resolved for the pattern's contract is
   every specialization's, but a dependent label has to be substituted
   first, as a contract wrapper does (maybe_instantiate_contracts).  */

static void
prepare_call_analysis (tree callee)
{
  if (!flag_contracts_p4405)
    return;
  tree origin = DECL_ORIGIN (callee);
  tree specs = get_fn_contract_specifiers (origin);
  if (!specs)
    return;
  for (tree contract : tree_vec_range (specs))
    if (PRECONDITION_P (contract)
	&& CONTRACT_LABEL (contract)
	&& type_dependent_expression_p (CONTRACT_LABEL (contract)))
      {
	maybe_instantiate_contracts (origin, /*for_wrapper=*/true);
	return;
      }
}

/* D4405: the semantic of the analysis evaluation of CONTRACT, a contract of
   CALLEE, at a call of CALLEE whose caller-side resolution gave it
   CALLER_ANALYSIS (caller_resolution::analysis, DECISIONS.md E142).  A
   precondition's analysis evaluation is at each call, so it is the
   stronger of that and the precondition's own analysis semantic, resolved
   as for CALLEE's definition: no match key makes it depend on the caller.
   That of a postcondition is in CALLEE's body; at the call there is only
   what a caller-side prove or diagnose gives it.  */

static contract_evaluation_semantic
call_analysis_semantic (tree contract, tree callee,
			contract_evaluation_semantic caller_analysis)
{
  if (!PRECONDITION_P (contract)
      || !flag_contracts_p4405
      || CONTRACT_CONDITION (contract) == error_mark_node
      || (CONTRACT_LABEL (contract)
	  && type_dependent_expression_p (CONTRACT_LABEL (contract))))
    return caller_analysis;
  tree origin = DECL_ORIGIN (callee);
  /* Nothing to resolve when neither prove nor diagnose is allowed.  */
  if (!(make_contract_query (contract, origin).allowed_mask
	& CES_PROVING_MASK))
    return caller_analysis;
  return contract_analysis_stronger (caller_analysis,
				     ensure_analysis_semantic (contract,
							       origin));
}

/* D4405 ("Taking the Address of a Function"): FN's address is being taken
   at LOC -- by the unary & operator, the function-to-pointer conversion,
   forming a pointer to member function, or binding a reference to FN.  A
   call through the result cannot identify FN, so it has no analysis
   evaluation, and a precondition of FN can be proven for it only if it is
   tautological.  How hard to try, and when to report a precondition that
   is not, is the implementation's: this one diagnoses only where it is
   known that calls through the address need a proof -- when the labels of
   a precondition allow no runtime semantic (always_prove, always_diagnose),
   so that its analysis evaluation is prove or diagnose in every
   translation unit and nothing checks it at run time.  Unless the proving
   strategies prove it with nothing known of the arguments (only folding
   can: contract_prove, with no call and no marker), that is an error for
   prove and a -Wcontract-unproven warning for diagnose, saying that the
   precondition cannot be proven, or is always false, for a call through
   the address -- taking the address is not itself ill-formed.  When only
   the configuration selects prove or diagnose, nothing is diagnosed: this
   translation unit's configuration does not determine what calls through
   the address are, and calls through it simply go unanalyzed.  The
   analysis semantic of a precondition is the same at every call
   (call_analysis_semantic), so it is known here.  Nothing is diagnosed in
   a template, whose instantiation takes the address, nor in an unevaluated
   operand, nor without tf_error (overload resolution trying the
   conversion), nor for the address a direct call calls
   (contract_forming_direct_call, set by build_addr_func).

   A default argument takes the address at each call that uses it, but its
   semantic constraints are checked where it is written ([dcl.fct.default]),
   so that is where it is diagnosed, once, whether or not a call uses it:
   when check_default_argument converts it to the parameter's type
   (contract_checking_default_argument, an unevaluated operand there), and
   not when a call converts it again (contract_using_default_argument, set
   by convert_default_arg) -- each for that scope only, not for what an
   instantiation it causes builds (push_to_top_level).  A template's is checked as it is instantiated
   (tsubst_default_argument).

   STATIC_VAR, if not null, is the object of static or thread storage
   duration whose definition takes the address of its destructor FN
   (contract_diagnose_static_destructor), named in a note -- or, when
   EXTENDING_VAR is not null, the temporary whose lifetime the reference
   EXTENDING_VAR extends, which the note names by it.  */

bool contract_forming_direct_call;
saved_scope *contract_checking_default_argument;
saved_scope *contract_using_default_argument;

/* The note naming STATIC_VAR, if not null, at LOC: its definition takes the
   address of its destructor (contract_diagnose_static_destructor).  If
   EXTENDING_VAR is not null, STATIC_VAR is a temporary whose lifetime it
   extends.  */

static void
inform_static_destructor_address (location_t loc, tree static_var,
				  tree extending_var)
{
  if (!static_var)
    return;
  if (extending_var)
    {
      /* The temporary has the storage duration of the variable.  */
      if (CP_DECL_THREAD_LOCAL_P (extending_var))
	inform (loc, "a temporary lifetime-extended by %qD, of thread storage "
		"duration, is destroyed by a call through the address of its "
		"destructor", extending_var);
      else
	inform (loc, "a temporary lifetime-extended by %qD, of static storage "
		"duration, is destroyed by a call through the address of its "
		"destructor", extending_var);
    }
  else if (CP_DECL_THREAD_LOCAL_P (static_var))
    inform (loc, "%qD, of thread storage duration, is destroyed by a call "
	    "through the address of its destructor", static_var);
  else
    inform (loc, "%qD, of static storage duration, is destroyed by a call "
	    "through the address of its destructor", static_var);
}

void
contract_diagnose_address_taken (tree fn, location_t loc,
				 tsubst_flags_t complain, tree static_var,
				 tree extending_var)
{
  if (!flag_contracts_p4405
      || contract_forming_direct_call
      || contract_using_default_argument == scope_chain
      || processing_template_decl
      || (cp_unevaluated_operand
	  && contract_checking_default_argument != scope_chain)
      || !(complain & tf_error)
      || TREE_CODE (fn) != FUNCTION_DECL
      || CONTRACT_HELPER (fn) != ldf_contract_none)
    return;
  /* A lambda's conversion function is defined with the lambda, whether or
     not anything converts it.  */
  if (current_function_decl
      && DECL_CONV_FN_P (current_function_decl)
      && LAMBDA_TYPE_P (DECL_CONTEXT (current_function_decl)))
    return;
  tree origin = DECL_ORIGIN (fn);
  if (!DECL_HAS_CONTRACTS_P (origin))
    return;
  prepare_call_analysis (fn);
  tree specs = get_fn_contract_specifiers (origin);
  if (!specs)
    return;
  /* The first precondition not proven with prove, else the first with
     diagnose, is the one reported.  */
  tree unproven = NULL_TREE;
  bool unproven_prove = false, unproven_false = false;
  for (tree contract : tree_vec_range (specs))
    {
      if (!PRECONDITION_P (contract)
	  || CONTRACT_CONDITION (contract) == error_mark_node)
	continue;
      contract_evaluation_semantic analysis
	= call_analysis_semantic (contract, fn, CES_IGNORE);
      if (!contract_semantic_proving_p (analysis))
	continue;
      /* Only a precondition whose labels allow no runtime semantic.  */
      uint16_t mask = make_contract_query (contract, origin).allowed_mask;
      if (mask & ~CES_PROVING_MASK)
	continue;
      if (unproven && (unproven_prove || analysis != CES_PROVE))
	continue;
      tree proof = contract_for_caller_side_proof (origin, contract);
      if (!proof)
	continue;
      contract_proof_query query = {};
      query.opaque = proof == error_mark_node;
      query.predicate = (query.opaque ? CONTRACT_CONDITION (contract)
			 : CONTRACT_CONDITION (proof));
      query.semantic = analysis;
      query.kind = CAK_PRE;
      query.loc = loc;
      contract_proof_result r = contract_prove (query);
      if (contract_proof_proven_p (r))
	continue;
      unproven = contract;
      unproven_prove = analysis == CES_PROVE;
      unproven_false = r == CPR_ALWAYS_FALSE;
    }
  if (!unproven)
    return;
  auto_diagnostic_group d;
  bool emitted = true;
  if (unproven_prove)
    {
      if (unproven_false)
	error_at (loc, "contract predicate is always false for a call "
		  "through the address of %qD", fn);
      else
	error_at (loc, "contract assertion cannot be proven for a call "
		  "through the address of %qD", fn);
    }
  else if (unproven_false)
    emitted = warning_at (loc, OPT_Wcontract_unproven,
			  "contract predicate is always false for a call "
			  "through the address of %qD", fn);
  else
    emitted = warning_at (loc, OPT_Wcontract_unproven,
			  "contract assertion cannot be proven for a call "
			  "through the address of %qD", fn);
  if (emitted)
    {
      inform_static_destructor_address (loc, static_var, extending_var);
      inform (EXPR_LOCATION (unproven), "precondition here");
    }
}

/* D4405R0 section 2.1: an object of static or thread storage duration is
   destroyed at exit, or when its thread ends, by a call through the
   address of its destructor (registered with __cxa_atexit or
   __cxa_thread_atexit), which no caller in the program makes to analyze.
   So the definition of DECL, such an object needing destruction
   (expand_static_init: not one with constant destruction), is treated as
   taking the address of its destructor (contract_diagnose_address_taken).
   A template's static is checked as it is instantiated, once.

   A temporary whose lifetime is extended to static or thread storage
   duration is destroyed the same way (DECISIONS.md E147 (a)): DECL is then
   the temporary's variable, EXTENDING_VAR the reference (or the object with
   a reference member, or the std::initializer_list) extending it, and LOC
   the temporary's location (set_up_extended_ref_temp).  */

void
contract_diagnose_static_destructor (tree decl, tree extending_var,
				     location_t loc)
{
  if (!flag_contracts_p4405 || processing_template_decl)
    return;
  tree type = strip_array_types (TREE_TYPE (decl));
  /* An implicitly-declared destructor has no contract assertions.  */
  if (!CLASS_TYPE_P (type)
      || TYPE_HAS_TRIVIAL_DESTRUCTOR (type)
      || CLASSTYPE_LAZY_DESTRUCTOR (type))
    return;
  tree dtor = CLASSTYPE_DESTRUCTOR (type);
  if (dtor && TREE_CODE (dtor) == FUNCTION_DECL)
    {
      if (loc == UNKNOWN_LOCATION)
	loc = DECL_SOURCE_LOCATION (decl);
      contract_diagnose_address_taken (dtor, loc, tf_warning_or_error, decl,
				       extending_var);
    }
}

/* D4405: converting a closure to a pointer to function, by calling its
   conversion function CONV, takes the address of its function call
   operator -- for a generic lambda, the specialization CONV converts to:
   a call through the pointer, or through the static `_FUN' it may point
   to, has no analysis evaluation.  Diagnosed at LOC.  */

void
contract_diagnose_lambda_conversion (tree conv, location_t loc,
				     tsubst_flags_t complain)
{
  if (!flag_contracts_p4405 || processing_template_decl)
    return;
  tree callop = lambda_function (DECL_CONTEXT (conv));
  if (!callop)
    return;
  if (generic_lambda_fn_p (callop))
    {
      if (!DECL_TEMPLATE_INFO (conv))
	return;
      callop = instantiate_template (DECL_TI_TEMPLATE (callop),
				     INNERMOST_TEMPLATE_ARGS
				       (DECL_TI_ARGS (conv)),
				     tf_none);
      if (callop == error_mark_node)
	return;
    }
  contract_diagnose_address_taken (callop, loc, complain);
}

/* D4405 ("The Analysis Evaluation"): there is no analysis evaluation
   during constant evaluation, so a call in a manifestly constant-evaluated
   expression has none (the constant evaluation itself evaluates a
   precondition resolved to prove or diagnose as enforce or observe,
   contract_constant_evaluation_semantic).  The parser and pt.cc mark such
   an expression of the current function, or of namespace scope, while
   they build its calls (contract_constant_evaluation_sentinel): the
   initializer of a constexpr or constinit variable, a constant-expression
   (a static_assert condition, a case label, an enumerator, a bit-field
   width ...), a template argument and an `if constexpr' condition.  Not a
   function body defined in one (a lambda's), which is another function,
   nor what an instantiation builds meanwhile (push_to_top_level).  So is
   every call in an immediate function, or in the consteval branch of `if
   consteval', and every call of an immediate function: each is evaluated
   only by constant evaluation.  A constant initialization of a variable
   that is not constexpr is found only when it is made, and drops the proof
   markers of the calls it replaces (contract_object_proof_markers).  */

static bool contract_constant_evaluation_active;
static tree contract_constant_evaluation_fn;
static saved_scope *contract_constant_evaluation_scope;

contract_constant_evaluation_sentinel::contract_constant_evaluation_sentinel
  (bool active)
  : saved_active (contract_constant_evaluation_active),
    saved_fn (contract_constant_evaluation_fn),
    saved_chain (contract_constant_evaluation_scope)
{
  if (active)
    {
      contract_constant_evaluation_active = true;
      contract_constant_evaluation_fn = current_function_decl;
      contract_constant_evaluation_scope = scope_chain;
    }
}

contract_constant_evaluation_sentinel::~contract_constant_evaluation_sentinel ()
{
  contract_constant_evaluation_active = saved_active;
  contract_constant_evaluation_fn = saved_fn;
  contract_constant_evaluation_scope = saved_chain;
}

/* True if a call of CALLEE built now is manifestly constant-evaluated, as
   above.  */

static bool
contract_constant_evaluation_p (tree callee)
{
  return ((contract_constant_evaluation_active
	   && contract_constant_evaluation_fn == current_function_decl
	   && contract_constant_evaluation_scope == scope_chain)
	  || (current_function_decl
	      && DECL_IMMEDIATE_FUNCTION_P (current_function_decl))
	  || in_consteval_if_p
	  || DECL_IMMEDIATE_FUNCTION_P (callee));
}

/* P4405: diagnose the analysis evaluations of a call of CALLEE with the
   prove or diagnose semantic (call_analysis_semantic), which happen now,
   unless proven: resolved for the call site (TUPLE_LOC, the current
   function) and diagnosed at LOC.  CALL, if non-null, is a call whose
   arguments are CALLEE's own, for the proof.  */

static void
diagnose_caller_side_proving (tree callee, tree call, location_t tuple_loc,
			      location_t loc)
{
  prepare_call_analysis (callee);
  auto_vec<contract_evaluation_semantic> analyses;
  tree tuple = compute_caller_semantic_tuple (callee, tuple_loc,
					      current_function_decl,
					      &analyses);
  tree specs = get_fn_contract_specifiers (DECL_ORIGIN (callee));
  unsigned position = 0;
  for (tree t = tuple; t; t = TREE_CHAIN (t), position++)
    {
      tree contract = TREE_VEC_ELT (specs, position);
      contract_evaluation_semantic sem
	= call_analysis_semantic (contract, callee, analyses[position]);
      if (!contract_semantic_proving_p (sem))
	continue;
      if (CONTRACT_CONDITION (contract) == error_mark_node)
	continue;
      tree proof = contract_for_caller_side_proof (DECL_ORIGIN (callee),
						   contract);
      if (proof)
	diagnose_unproven_contract (proof == error_mark_node ? contract : proof,
				    sem, loc, callee, call, NULL, NULL_TREE,
				    proof == error_mark_node);
    }
}

/* P4405: a call of an inheriting constructor is a call of the constructor
   it inherits ([class.inhctor.init]), so that constructor's prove and
   diagnose analysis evaluations happen at CALL, the call of FNDECL, and
   are diagnosed there, with the call's arguments for the parameters.  The
   inheriting constructor GCC defines calls the inherited one, and does not
   diagnose them again (maybe_contract_wrap_call).  IN_PREDICATE is true
   for a call in a contract predicate that a check evaluates
   (diagnose_checked_predicate_calls).  */

static void
diagnose_inheriting_ctor_call (tree fndecl, tree call,
			       bool in_predicate = false)
{
  tree inh = inherited_ctor_for_proving (fndecl);
  if (!inh || !handle_caller_contracts_p (inh))
    return;
  if (!in_predicate && (cp_unevaluated_operand || processing_contract_condition
			|| contract_constant_evaluation_p (fndecl)))
    return;
  /* Nor is the call diagnosed in an inheriting constructor of a further
     derived class, which calls this one for its own caller.  */
  if (current_function_decl
      && inherited_ctor_for_proving (current_function_decl) == inh)
    return;
  /* What the odr-use of INH would do (mark_used), which the inheriting
     constructor's own definition does only later.  */
  maybe_instantiate_contracts (inh);
  /* The call passes the derived object, which the inherited constructor's
     `this' sees converted to its base, and then the arguments; an
     in-charge or VTT argument of FNDECL would get in the way.  */
  tree icall = NULL_TREE;
  if (TREE_CODE (call) == CALL_EXPR && call_expr_nargs (call) > 0
      && !DECL_HAS_IN_CHARGE_PARM_P (fndecl)
      && !DECL_HAS_VTT_PARM_P (fndecl))
    {
      tree obj = CALL_EXPR_ARG (call, 0);
      tree base = NULL_TREE;
      if (TYPE_PTR_P (TREE_TYPE (obj)))
	base = lookup_base (TREE_TYPE (TREE_TYPE (obj)), DECL_CONTEXT (inh),
			    ba_any, NULL, tf_none);
      if (base && base != error_mark_node)
	obj = build_base_path (PLUS_EXPR, obj, base, 1, tf_none);
      if (base && base != error_mark_node && obj != error_mark_node)
	{
	  icall = copy_node (call);
	  CALL_EXPR_ARG (icall, 0) = obj;
	}
    }
  location_t loc = (in_predicate
		    ? EXPR_LOC_OR_LOC (call, input_location)
		    : cp_expr_loc_or_input_loc (call));
  diagnose_caller_side_proving (inh, icall,
				in_predicate ? loc : input_location, loc);
}

/* walk_tree callback for diagnose_checked_predicate_calls.  */

static tree
diagnose_checked_predicate_calls_r (tree *tp, int *walk_subtrees, void *)
{
  tree t = *tp;
  if (TYPE_P (t) || TREE_CODE (t) == LAMBDA_EXPR)
    {
      /* A lambda's body is a function of its own, whose calls were
	 diagnosed with it.  */
      *walk_subtrees = 0;
      return NULL_TREE;
    }
  if (TREE_CODE (t) != CALL_EXPR && TREE_CODE (t) != AGGR_INIT_EXPR)
    return NULL_TREE;
  tree fn = cp_get_callee_fndecl_nofold (t);
  if (!fn)
    return NULL_TREE;
  if (tree orig = get_orig_func_for_wrapper (fn))
    fn = orig;
  if (TREE_CODE (t) == CALL_EXPR && inherited_ctor_for_proving (fn))
    diagnose_inheriting_ctor_call (fn, t, /*in_predicate=*/true);
  else if (handle_caller_contracts_p (fn))
    {
      location_t loc = EXPR_LOC_OR_LOC (t, input_location);
      diagnose_caller_side_proving (fn, TREE_CODE (t) == CALL_EXPR ? t
				    : NULL_TREE, loc, loc);
    }
  return NULL_TREE;
}

/* P4405: CONTRACT's predicate is about to be evaluated by a check (or, for
   a dynamically selected semantic, may be), so the prove and diagnose
   analysis evaluations of the calls in it happen too: diagnose them, here.
   Where the predicate was parsed nothing could be diagnosed, as nothing
   was known of the semantics it would be evaluated with
   (maybe_contract_wrap_call).  */

static void
diagnose_checked_predicate_calls (tree contract)
{
  if (!flag_contracts_p4405 || CONTRACT_CONDITION (contract) == error_mark_node)
    return;
  cp_walk_tree_without_duplicates (&CONTRACT_CONDITION (contract),
				   diagnose_checked_predicate_calls_r, NULL);
}

/* P4405 proving hook: true if T is the proof markers that
   maybe_contract_wrap_call places ahead of a call -- the regions of
   contract_prove_shadow_strategy's shadow_mark, each `if
   (.CONTRACT_SHADOW_GUARD (0)) ...' and possibly followed by an `.ASSUME',
   chained with COMPOUND_EXPRs.  */

bool
contract_proof_markers_p (const_tree t)
{
  switch (TREE_CODE (t))
    {
    case COMPOUND_EXPR:
      return (contract_proof_markers_p (TREE_OPERAND (t, 0))
	      && contract_proof_markers_p (TREE_OPERAND (t, 1)));
    case COND_EXPR:
      {
	const_tree c = TREE_OPERAND (t, 0);
	return (TREE_CODE (c) == CALL_EXPR
		&& CALL_EXPR_FN (c) == NULL_TREE
		&& CALL_EXPR_IFN (c) == IFN_CONTRACT_SHADOW_GUARD);
      }
    case CALL_EXPR:
      return CALL_EXPR_FN (t) == NULL_TREE && CALL_EXPR_IFN (t) == IFN_ASSUME;
    default:
      return false;
    }
}

/* P4405 proving hook: T without the proof markers that
   maybe_contract_wrap_call puts ahead of a call in the initializer of its
   temporary, for a caller looking for the call there.  */

tree
contract_strip_proof_markers (tree t)
{
  while (TREE_CODE (t) == COMPOUND_EXPR
	 && contract_proof_markers_p (TREE_OPERAND (t, 0)))
    t = TREE_OPERAND (t, 1);
  return t;
}

/* P4405 proving hook: the proof markers that maybe_contract_wrap_call put
   in the object argument of CALL, a constructor call, or NULL_TREE.  A
   caller that replaces such a call by an initialization of the object --
   a constexpr constructor call folded to a constant -- keeps them.  */

tree
contract_object_proof_markers (tree call)
{
  if (TREE_CODE (call) != CALL_EXPR || call_expr_nargs (call) == 0)
    return NULL_TREE;
  tree arg = CALL_EXPR_ARG (call, 0);
  if (TREE_CODE (arg) == COMPOUND_EXPR
      && contract_proof_markers_p (TREE_OPERAND (arg, 0)))
    return TREE_OPERAND (arg, 0);
  return NULL_TREE;
}

/* walk_tree callback: find what a copy of a proof marker cannot share
   with the original -- a temporary, a saved expression or a
   declaration.  */

static tree
find_unsafe_copy_r (tree *tp, int *walk_subtrees, void *)
{
  if (TYPE_P (*tp))
    *walk_subtrees = 0;
  else if (TREE_CODE (*tp) == TARGET_EXPR || TREE_CODE (*tp) == SAVE_EXPR
	   || TREE_CODE (*tp) == BIND_EXPR || TREE_CODE (*tp) == DECL_EXPR)
    return *tp;
  return NULL_TREE;
}

/* walk_tree callback for contract_potential_cleanup_markers: DATA, a tree
   *, accumulates the proof-marker regions of each destructor call in the
   cleanup, not inside anything that binds a variable of its own (an array's
   loop, a temporary) -- those regions could name it.  An `.ASSUME' is left
   out: it belongs to the call.  */

static tree
contract_potential_cleanup_markers_r (tree *tp, int *walk_subtrees,
				      void *data)
{
  tree t = *tp;
  if (TYPE_P (t))
    {
      *walk_subtrees = 0;
      return NULL_TREE;
    }
  switch (TREE_CODE (t))
    {
    case BIND_EXPR:
    case LOOP_EXPR:
    case TARGET_EXPR:
    case DECL_EXPR:
    case STATEMENT_LIST:
      *walk_subtrees = 0;
      return NULL_TREE;
    default:
      break;
    }
  tree markers = contract_object_proof_markers (t);
  if (!markers)
    return NULL_TREE;
  /* Not into the call's object argument, which holds the markers.  */
  *walk_subtrees = 0;
  tree *acc = (tree *) data;
  auto_vec<tree, 4> todo;
  todo.safe_push (markers);
  while (!todo.is_empty ())
    {
      tree m = todo.pop ();
      if (TREE_CODE (m) == COMPOUND_EXPR)
	{
	  todo.safe_push (TREE_OPERAND (m, 1));
	  todo.safe_push (TREE_OPERAND (m, 0));
	}
      else if (TREE_CODE (m) == COND_EXPR
	       && !cp_walk_tree_without_duplicates (&m, find_unsafe_copy_r,
						     NULL))
	*acc = (*acc ? build2 (COMPOUND_EXPR, void_type_node, *acc,
			       unshare_expr (m))
		: unshare_expr (m));
    }
  return NULL_TREE;
}

/* D4405 (GCC-949): a contract_no_proof_deferral_sentinel is active while
   building the EH-only cleanup of an array's elements built so far
   (build_vec_init), or of an array subobject an aggregate initialization
   has built (maybe_push_temp_cleanup).  Its destructor calls sit in a loop,
   where contract_potential_cleanup_markers could not copy their markers
   out, so their analysis evaluations are decided at once, as at -O0.  Not
   those of a function defined meanwhile (an implicit destructor), as for
   contract_temporary_cleanup_sentinel.  */

static bool contract_no_proof_deferral_active;
static tree contract_no_proof_deferral_fn;
static saved_scope *contract_no_proof_deferral_scope;

contract_no_proof_deferral_sentinel::contract_no_proof_deferral_sentinel
  (bool active)
  : saved_active (contract_no_proof_deferral_active),
    saved_fn (contract_no_proof_deferral_fn),
    saved_chain (contract_no_proof_deferral_scope)
{
  if (active)
    {
      contract_no_proof_deferral_active = true;
      contract_no_proof_deferral_fn = current_function_decl;
      contract_no_proof_deferral_scope = scope_chain;
    }
}

contract_no_proof_deferral_sentinel::~contract_no_proof_deferral_sentinel ()
{
  contract_no_proof_deferral_active = saved_active;
  contract_no_proof_deferral_fn = saved_fn;
  contract_no_proof_deferral_scope = saved_chain;
}

/* True if a contract_no_proof_deferral_sentinel covers the calls built
   now.  */

static bool
contract_no_proof_deferral_p ()
{
  return (contract_no_proof_deferral_active
	  && contract_no_proof_deferral_fn == current_function_decl
	  && contract_no_proof_deferral_scope == scope_chain);
}

/* D4405 ("The Analysis Evaluation"; DECISIONS.md E160 (a), GCC-949):
   CLEANUP is the EH-only cleanup of a subobject an aggregate initialization
   has just built (maybe_push_temp_cleanup), whose destructor is
   potentially invoked until the initialization completes, whether or not
   anything after it can throw.  A proving strategy decides the analysis
   evaluations of its destructor call where their markers are, in the
   cleanup, which a shadow removes as unreachable when nothing can throw --
   proving them, though they are diagnosed at -O0, where folding decides
   them as the cleanup is built.  So a copy of those markers is placed in
   the initialization itself, where the subobject is complete: the same
   obligations (an obligation is unproven if any reachable copy of its
   marker is), never evaluated by the real code.  Return that copy, or
   NULL_TREE.  */

tree
contract_potential_cleanup_markers (tree cleanup)
{
  if (!flag_contracts_p4405 || !cleanup)
    return NULL_TREE;
  tree markers = NULL_TREE;
  cp_walk_tree_without_duplicates (&cleanup,
				   contract_potential_cleanup_markers_r,
				   &markers);
  return markers;
}

/* walk_tree callback: find the PARM_DECL DATA.  */

static tree
find_parm_r (tree *tp, int *walk_subtrees, void *data)
{
  if (TYPE_P (*tp))
    *walk_subtrees = 0;
  else if (*tp == (tree) data)
    return *tp;
  return NULL_TREE;
}

/* D4405: diagnose the analysis evaluations at CALL, a call of FNDECL
   located at LOC, unless proven: each precondition's, and what a
   caller-side prove or diagnose gives any contract (call_analysis_semantic,
   from ANALYSES, the caller-side resolution's).  None when SKIP.  When
   MARKER_CALL_P, a precondition's may be deferred to a proving strategy;
   return the markers of those that are, or NULL_TREE.  PLACEHOLDER_CTOR,
   *TEMP and *THIS_CALL are maybe_contract_wrap_call's.  */

static tree
contract_analyze_call (tree fndecl, tree call, location_t loc,
		       const vec<contract_evaluation_semantic> &analyses,
		       bool skip, bool marker_call_p, bool placeholder_ctor,
		       tree *temp, tree *this_call)
{
  tree proof_markers = NULL_TREE;
  tree specs = get_fn_contract_specifiers (DECL_ORIGIN (fndecl));
  unsigned nspecs = specs ? TREE_VEC_LENGTH (specs) : 0;
  for (unsigned i = 0; i < nspecs; i++)
    {
      tree contract = TREE_VEC_ELT (specs, i);
      contract_evaluation_semantic sem
	= call_analysis_semantic (contract, fndecl,
				  (i < analyses.length ()
				   ? analyses[i] : CES_IGNORE));
      if (skip
	  || !contract_semantic_proving_p (sem)
	  || CONTRACT_CONDITION (contract) == error_mark_node)
	continue;
      tree proof = contract_for_caller_side_proof (DECL_ORIGIN (fndecl),
						   contract);
      if (!proof)
	continue;
      bool opaque = proof == error_mark_node;
      if (!opaque)
	contract = proof;
      tree marker = NULL_TREE;
      tree pred = NULL_TREE;
      bool deferrable = (marker_call_p && PRECONDITION_P (contract)
			 && !opaque && !contract_no_proof_deferral_p ());
      if (deferrable)
	{
	  /* A predicate parsed on a declaration names a placeholder `this';
	     the marker needs the callee's own, for which the call's object
	     is substituted.  */
	  pred = unshare_expr (CONTRACT_CONDITION (contract));
	  remap_dummy_this (DECL_ORIGIN (fndecl), &pred);
	  if (placeholder_ctor
	      && cp_walk_tree_without_duplicates
		   (&pred, find_parm_r,
		    DECL_ARGUMENTS (DECL_ORIGIN (fndecl))))
	    {
	      /* The temporary the call initializes is its object.  */
	      if (!*temp)
		*temp = build_cplus_new (TREE_TYPE (TREE_TYPE
						    (CALL_EXPR_ARG (call, 0))),
					 call, tf_none);
	      if (TREE_CODE (*temp) != TARGET_EXPR)
		deferrable = false;
	      else if (!*this_call)
		{
		  *this_call = copy_node (call);
		  CALL_EXPR_ARG (*this_call, 0)
		    = build_address (TARGET_EXPR_SLOT (*temp));
		}
	    }
	}
      diagnose_unproven_contract (contract, sem, loc, fndecl,
				  deferrable && *this_call ? *this_call : call,
				  deferrable ? &marker : NULL, pred, opaque);
      if (marker)
	proof_markers = (proof_markers
			 ? build2 (COMPOUND_EXPR, void_type_node,
				   proof_markers, marker)
			 : marker);
    }
  return proof_markers;
}

/* D4405 (GCC-944): the destructor call in a temporary's cleanup is made
   only if the temporary is materialized, and an initialization elides it
   (TARGET_EXPR_ELIDING_P), which is known only when the function holding
   it is complete.  So the cleanup's call has its analysis evaluations
   (contract_analyze_call) then, not as it is built: build_target_expr
   builds the cleanup under a contract_temporary_cleanup_sentinel, and
   maybe_contract_wrap_call puts in the call's object argument, ahead of
   the object, `.CONTRACT_PENDING_ANALYSIS (ID)', ID indexing a record of
   what it resolved at the call.  When the function is genericized
   (cp_genericize_r) the record of an elided temporary's cleanup is
   dropped (contract_drop_pending_analyses) and every other is analyzed
   (contract_resolve_pending_analysis) -- in a function that is never
   compiled, too, as any other analysis evaluation -- in the function that
   built the call, which may defer them to a proving strategy, and
   otherwise in the context it was built in (for a namespace-scope
   variable, at namespace scope, though the static initialization
   function makes the call), where they are decided at once.  A copy of
   the cleanup (break_out_target_exprs) shares the record and its
   markers.  One that genericization does not reach is analyzed when it
   is gimplified, which keeps it.  */

struct GTY(()) contract_pending_analysis
{
  /* The destructor, and the function the call was built in (null at
     namespace scope).  */
  tree callee;
  tree fn;
  /* The caller-side resolution's analysis semantics, a TREE_VEC of
     INTEGER_CSTs, and the markers of those deferred.  */
  tree analyses;
  tree markers;
  location_t loc;
  /* c_contract_diagnostic_context's handle, or 0.  */
  unsigned context;
  bool resolved;
};

static GTY(()) vec<contract_pending_analysis, va_gc>
  *contract_pending_analyses;

static bool contract_temporary_cleanup_active;
static tree contract_temporary_cleanup_fn;
static saved_scope *contract_temporary_cleanup_scope;

contract_temporary_cleanup_sentinel::contract_temporary_cleanup_sentinel ()
  : saved_active (contract_temporary_cleanup_active),
    saved_fn (contract_temporary_cleanup_fn),
    saved_chain (contract_temporary_cleanup_scope)
{
  contract_temporary_cleanup_active = true;
  contract_temporary_cleanup_fn = current_function_decl;
  contract_temporary_cleanup_scope = scope_chain;
}

contract_temporary_cleanup_sentinel::~contract_temporary_cleanup_sentinel ()
{
  contract_temporary_cleanup_active = saved_active;
  contract_temporary_cleanup_fn = saved_fn;
  contract_temporary_cleanup_scope = saved_chain;
}

/* True if a destructor call built now is a temporary's cleanup: not one a
   function the cleanup needs builds meanwhile (an implicit destructor's
   definition), nor an instantiation (push_to_top_level).  Not while
   writing a precompiled header, which would keep the records but not the
   contexts.  */

static bool
contract_temporary_cleanup_p ()
{
  return (contract_temporary_cleanup_active
	  && contract_temporary_cleanup_fn == current_function_decl
	  && contract_temporary_cleanup_scope == scope_chain
	  && !pch_file);
}

/* True if a call of FNDECL has an analysis evaluation to diagnose, given
   ANALYSES (contract_analyze_call).  */

static bool
contract_call_analyzed_p (tree fndecl,
			  const vec<contract_evaluation_semantic> &analyses)
{
  tree specs = get_fn_contract_specifiers (DECL_ORIGIN (fndecl));
  unsigned nspecs = specs ? TREE_VEC_LENGTH (specs) : 0;
  for (unsigned i = 0; i < nspecs; i++)
    {
      tree contract = TREE_VEC_ELT (specs, i);
      if (CONTRACT_CONDITION (contract) != error_mark_node
	  && contract_semantic_proving_p
	       (call_analysis_semantic (contract, fndecl,
					(i < analyses.length ()
					 ? analyses[i] : CES_IGNORE))))
	return true;
    }
  return false;
}

/* The `.CONTRACT_PENDING_ANALYSIS (ID)' of a destructor call of FNDECL in
   a temporary's cleanup, located at LOC, whose caller-side resolution gave
   ANALYSES.  */

static tree
contract_defer_call_analysis
  (tree fndecl, location_t loc,
   const vec<contract_evaluation_semantic> &analyses)
{
  tree sems = make_tree_vec (analyses.length ());
  for (unsigned i = 0; i < analyses.length (); i++)
    TREE_VEC_ELT (sems, i) = build_int_cst (unsigned_type_node, analyses[i]);
  contract_pending_analysis p = {};
  p.callee = fndecl;
  p.fn = current_function_decl;
  p.analyses = sems;
  p.loc = loc;
  p.context = c_contract_diagnostic_context ();
  unsigned id = vec_safe_length (contract_pending_analyses);
  vec_safe_push (contract_pending_analyses, p);
  return build_call_expr_internal_loc (loc, IFN_CONTRACT_PENDING_ANALYSIS,
				       void_type_node, 1,
				       build_int_cst (unsigned_type_node, id));
}

/* If CALL's object argument holds a `.CONTRACT_PENDING_ANALYSIS (ID)',
   return ID's record and remove it from there; otherwise null.  */

static contract_pending_analysis *
contract_take_pending_analysis (tree call)
{
  if (TREE_CODE (call) != CALL_EXPR || call_expr_nargs (call) == 0)
    return NULL;
  tree arg = CALL_EXPR_ARG (call, 0);
  if (TREE_CODE (arg) != COMPOUND_EXPR)
    return NULL;
  tree p = TREE_OPERAND (arg, 0);
  if (TREE_CODE (p) != CALL_EXPR
      || CALL_EXPR_FN (p) != NULL_TREE
      || CALL_EXPR_IFN (p) != IFN_CONTRACT_PENDING_ANALYSIS)
    return NULL;
  unsigned id = tree_to_uhwi (CALL_EXPR_ARG (p, 0));
  CALL_EXPR_ARG (call, 0) = TREE_OPERAND (arg, 1);
  if (id >= vec_safe_length (contract_pending_analyses))
    return NULL;
  return &(*contract_pending_analyses)[id];
}

/* walk_tree callback: drop the pending analyses (a cleanup that is never
   made).  */

static tree
contract_drop_pending_analyses_r (tree *tp, int *walk_subtrees, void *)
{
  if (TYPE_P (*tp))
    *walk_subtrees = 0;
  else
    contract_take_pending_analysis (*tp);
  return NULL_TREE;
}

/* D4405 (GCC-944): T is a TARGET_EXPR being genericized.  If an
   initialization elides it, drop the pending analyses of its cleanup,
   whose calls are never made.  */

void
contract_drop_pending_analyses (tree t)
{
  if (vec_safe_is_empty (contract_pending_analyses)
      || !TARGET_EXPR_CLEANUP (t)
      || !TARGET_EXPR_ELIDING_P (t)
      /* Only a temporary of a type passed by invisible reference -- every
	 class with a non-trivial destructor -- is always elided so (the
	 assertions in cp_gimplify_init_expr and cp_gimplify_expr).  */
      || !TREE_ADDRESSABLE (TREE_TYPE (t)))
    return;
  cp_walk_tree_without_duplicates (&TARGET_EXPR_CLEANUP (t),
				   contract_drop_pending_analyses_r, NULL);
}

/* D4405 (GCC-944): CALL, being genericized or gimplified in the current
   function, is a call that is made.  If its object argument holds a
   pending analysis, analyze it now and put the markers of those deferred
   in its place.  */

void
contract_resolve_pending_analysis (tree call)
{
  if (vec_safe_is_empty (contract_pending_analyses))
    return;
  contract_pending_analysis *p = contract_take_pending_analysis (call);
  if (!p)
    return;
  bool here = p->fn == current_function_decl;
  tree markers = NULL_TREE;
  if (p->resolved)
    markers = here ? p->markers : NULL_TREE;
  else
    {
      p->resolved = true;
      auto_vec<contract_evaluation_semantic> analyses;
      for (tree s : tree_vec_range (p->analyses))
	analyses.safe_push ((contract_evaluation_semantic) tree_to_uhwi (s));
      iloc_sentinel ils (p->loc);
      if (p->context)
	c_contract_switch_diagnostic_context (p->context, true);
      /* Elsewhere, in the context the call was built in, with no function
	 to defer to.  */
      if (!here)
	{
	  push_cfun (NULL);
	  current_function_decl = p->fn;
	}
      bool marker_call_p = here && contract_proof_deferral_ok_p (p->fn);
      tree temp = NULL_TREE, this_call = NULL_TREE;
      markers = contract_analyze_call (p->callee, call, p->loc, analyses,
				       /*skip=*/false, marker_call_p,
				       /*placeholder_ctor=*/false,
				       &temp, &this_call);
      if (!here)
	pop_cfun ();
      if (p->context)
	c_contract_switch_diagnostic_context (p->context, false);
      p->markers = markers;
    }
  if (markers)
    {
      tree arg = CALL_EXPR_ARG (call, 0);
      CALL_EXPR_ARG (call, 0)
	= build2 (COMPOUND_EXPR, TREE_TYPE (arg), unshare_expr (markers),
		  arg);
    }
}

/* Return true if any contract of FNDECL is checked or assumed under the
   current build configuration.  */

static bool
contract_any_active_p (tree fndecl)
{
  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return false;

  for (tree contract : tree_vec_range (contracts))
    if (contract_active_p (contract, fndecl))
      return true;
  return false;
}

/* True if FNDECL has any checked contracts whose TREE_CODE is
   C.  */

static bool
has_active_contract_condition (tree fndecl, tree_code c)
{
  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return false;

  for (tree contract : tree_vec_range (contracts))
    if (TREE_CODE (contract) == c && contract_active_p (contract, fndecl))
      return true;
  return false;
}

/* True if FNDECL has any checked or assumed preconditions.  */

static bool
has_active_preconditions (tree fndecl)
{
  return has_active_contract_condition (fndecl, PRECONDITION_STMT);
}

/* True if FNDECL has any checked or assumed postconditions.  */

static bool
has_active_postconditions (tree fndecl)
{
  return has_active_contract_condition (fndecl, POSTCONDITION_STMT);
}

/* True if evaluating FNDECL's postconditions could exit via an exception, so
   the returned object -- which exists by the time they run -- has to be
   destroyed on that path.

   Only "observe" and "enforce" let an exception out of a handler call:
   "ignore" and "assume" never evaluate the predicate, "quick_enforce"
   terminates without calling the handler at all, and the P4298 "noexcept_"
   variants call it under noexcept, so a throw there terminates rather than
   propagating.  all_contracts_statically_nonthrowing already draws exactly
   that line, and is conservative about the two ways the semantic can change
   under us at runtime: a P3595 dynamic selector (CONTRACT_DYNAMIC) or an
   assertion-control label (CONTRACT_LABEL).

   Upstream has no per-assertion semantics to consult -- one global
   -fcontract-evaluation-semantic and no noexcept_ variants -- so it can only
   answer this for the whole translation unit.  */

static bool
postconditions_may_throw_p (tree fndecl)
{
  if (!flag_exceptions || !has_active_postconditions (fndecl))
    return false;

  return !all_contracts_statically_nonthrowing (get_fn_contract_specifiers
						(fndecl), fndecl,
						POSTCONDITION_STMT);
}

/* Return true if any contract in CONTRACTS is not yet parsed.  */

bool
contract_any_deferred_p (tree contracts)
{
  if (!contracts)
    return false;

  for (tree contract : tree_vec_range (contracts))
    if (CONTRACT_CONDITION_DEFERRED_P (contract))
      return true;
  return false;
}

/* Returns true if function decl FNDECL has contracts and we need to
   process them for the purposes of either building caller or definition
   contract checks.
   This function does not take into account whether caller or definition
   side checking is enabled. Those checks will be done from the calling
   function which will be able to determine whether it is doing caller
   or definition contract handling.  */

static bool
handle_contracts_p (tree fndecl)
{
  return (flag_contracts
	  && !processing_template_decl
	  && (CONTRACT_HELPER (fndecl) == ldf_contract_none)
	  && contract_any_active_p (fndecl));
}

/* Like handle_contracts_p, but for the caller-side wrapping decision.  A
   caller may enable checking (P3595 caller-side semantics) even when the
   callee's own evaluation semantic is "ignore", so gate only on the presence
   of contracts here; whether any caller-side semantic is actually active at a
   particular call site is decided per call site in maybe_contract_wrap_call.
   A constructor or destructor call names a clone, whose contracts are its
   origin's.  */

static bool
handle_caller_contracts_p (tree fndecl)
{
  return (flag_contracts
	  && !processing_template_decl
	  && (CONTRACT_HELPER (fndecl) == ldf_contract_none)
	  && DECL_HAS_CONTRACTS_P (DECL_CONSTRUCTOR_P (fndecl)
				   || DECL_DESTRUCTOR_P (fndecl)
				   ? DECL_ORIGIN (fndecl) : fndecl));
}

/* For use with the tree inliner. This preserves non-mapped local variables,
   such as postcondition result variables, during remapping.  */

static tree
retain_decl (tree decl, copy_body_data *)
{
  return decl;
}

/* Get constract_assertion_kind of the specified contract. Used when building
  contract_violation object.  */

static contract_assertion_kind
get_contract_assertion_kind (tree contract)
{
  if (CONTRACT_ASSERTION_KIND (contract))
    {
      tree s = CONTRACT_ASSERTION_KIND (contract);
      tree i = (TREE_CODE (s) == INTEGER_CST) ? s
					      : DECL_INITIAL (STRIP_NOPS (s));
      gcc_checking_assert (!type_dependent_expression_p (s) && i);
      return (contract_assertion_kind) tree_to_uhwi (i);
    }

  switch (TREE_CODE (contract))
  {
    case ASSERTION_STMT:	return CAK_ASSERT;
    case PRECONDITION_STMT:	return CAK_PRE;
    case POSTCONDITION_STMT:	return CAK_POST;
    default: break;
  }

  gcc_unreachable ();
}

/* LABEL as the facets see it: a const lvalue of its type, as the library
   concepts ask about (`const T&'), so a non-const member is not a facet even
   for a prvalue label such as pre<L{}>.  */

static tree
label_facet_object (tree label)
{
  tree type = TREE_TYPE (label);
  if (lvalue_p (label) && CP_TYPE_CONST_P (type))
    return label;
  tree cref = cp_build_reference_type (cp_build_qualified_type
				       (type, TYPE_QUAL_CONST), false);
  tree obj = build_static_cast (input_location, cref, label, tf_none);
  return obj == error_mark_node ? label : convert_from_reference (obj);
}

/* True when MEMBER, looked up in LABEL_TYPE, is public in it.

   Facet detection uses lookup_member with protect=0, which finds a member
   regardless of access.  An inaccessible member must not make the type
   participate in the facet: the library concept is a requires-expression, and
   that is false for a member the caller cannot name.  Detection therefore has
   to answer "absent" -- and specifically must not diagnose, which is what
   happened before, when the deferred access check from the viability probe
   escaped as an error.  P3400 is explicit that an inaccessible member is not
   ill-formed, precisely because a label may carry private helpers sharing a
   facet's name -- tag-dispatch overloads, say -- that were never meant to be
   the facet at all.

   Access is judged as the concept judges it, from namespace
   std::contracts::labels, not from the contract: a contract written in a
   member or friend of the label must not see a private facet.  */

static bool
label_facet_accessible_p (tree label_type, tree member)
{
  if (!member || member == error_mark_node
      || !label_type || !CLASS_TYPE_P (label_type) || !TYPE_BINFO (label_type))
    return false;

  tree decl = member;
  if (BASELINK_P (decl))
    decl = BASELINK_FUNCTIONS (decl);
  if (TREE_CODE (decl) == OVERLOAD)
    decl = OVL_FIRST (decl);
  if (TREE_CODE (decl) == TEMPLATE_DECL)
    decl = DECL_TEMPLATE_RESULT (decl);

  return accessible_p (TYPE_BINFO (label_type), decl,
		       /*consider_local_p=*/false);
}

/* Constant-evaluate CALL, an invocation of the facet named FACET_NAME on an
   assertion-control object of LABEL_TYPE, and diagnose at LOC if it is not
   a constant expression.

   By the time this is reached the facet is present: the member exists, is
   accessible, and the call is viable.  No concept can ask whether a member
   is usable in a constant expression, so per P3400 such a type still
   participates in the facet and the error arrives when the result is
   consumed during translation.  Say what is actually wrong -- naming the
   facet and the control object -- rather than letting the evaluator's
   generic "call to non-'constexpr' function" escape, which never mentions
   contracts and leaves the reader to work out why 'constexpr' is required
   here.  The facet and the type between them identify the member, so this
   is one diagnostic rather than ours followed by the evaluator's; Clang's
   err_contract_facet_not_constant says the same thing.  */

static tree
constant_facet_value (tree call, tree label_type, const char *facet_name,
		      location_t loc)
{
  tree result = cxx_constant_value (call, NULL_TREE, tf_none);
  if (result && result != error_mark_node)
    return result;

  /* Name the type without its cv-qualification: a label is always a
     constexpr, therefore const, object, so "const foo_t" here is noise that
     every one of these messages would carry.  */
  error_at (loc,
	    "the %qs facet of assertion-control object %qT must be usable "
	    "in a constant expression", facet_name,
	    TYPE_MAIN_VARIANT (label_type));
  return error_mark_node;
}

static tree find_std_contracts_type (tree);

/* The argument a compute_semantic facet is called with: a non-const lvalue of
   std::contracts::evaluation_semantic holding SEM, as in the library concept.
   Passing that and letting overload resolution choose is what makes a
   reference or deduced parameter, an explicit object parameter and an
   overload set all work.  NULL_TREE if <contracts> is not available: the
   enumeration is only looked for, never injected as lookup_std_contracts_type
   would inject a class (GCC-676).  */

static tree
build_semantic_facet_arg (uint16_t sem)
{
  tree sem_type
    = find_std_contracts_type (get_identifier ("evaluation_semantic"));
  if (!sem_type || TREE_CODE (sem_type) != ENUMERAL_TYPE)
    return NULL_TREE;
  tree tmp = force_target_expr (sem_type, build_int_cst (sem_type, sem),
				tf_none);
  return cp_build_fold_indirect_ref (build_address (tmp));
}

/* Build LABEL.compute_semantic(SEM_VAL), implicitly converted to
   std::contracts::evaluation_semantic.  Returns NULL_TREE if LABEL's type is
   not a computed-semantics control type ([basic.contract.control]): the
   member does not exist, cannot be called, is inaccessible, or its result is
   not implicitly convertible to evaluation_semantic, so that an int, another
   enumeration or a class with only an explicit conversion function does not
   participate, while a class with an implicit one does.  */

static tree
build_compute_semantic_call (tree label, uint16_t sem_val)
{
  if (!label || label == error_mark_node
      || !TREE_TYPE (label) || !CLASS_TYPE_P (TREE_TYPE (label))
      || type_dependent_expression_p (label))
    return NULL_TREE;
  tree label_type = TREE_TYPE (label);
  tree fn = lookup_member (label_type, get_identifier ("compute_semantic"),
			   /*protect=*/0, /*want_type=*/false, tf_none);
  if (!fn || fn == error_mark_node)
    return NULL_TREE;

  tree sem_arg = build_semantic_facet_arg (sem_val);
  if (!sem_arg)
    return NULL_TREE;
  tree sem_type = TREE_TYPE (sem_arg);
  vec<tree, va_gc> *args = NULL;
  vec_safe_push (args, sem_arg);
  /* Access is judged below, on the member chosen, as the concept's
     requires-expression would: a private overload sharing the facet's name
     must neither be the facet nor stop a public one from being it.  */
  push_deferring_access_checks (dk_no_check);
  tree call = build_new_method_call (label_facet_object (label), fn, &args,
				     NULL_TREE, LOOKUP_NORMAL, NULL, tf_none);
  pop_deferring_access_checks ();
  if (!call || call == error_mark_node)
    return NULL_TREE;
  tree callee = cp_get_callee_fndecl_nofold (extract_call_expr (call));
  if (!callee || !label_facet_accessible_p (label_type, callee))
    return NULL_TREE;
  /* The concept's { t.compute_semantic(s) } -> convertible_to<...>.  */
  if (!is_convertible (TREE_TYPE (call), sem_type))
    return NULL_TREE;
  tree conv = perform_implicit_conversion (sem_type, call, tf_none);
  if (!conv || conv == error_mark_node)
    return NULL_TREE;
  return conv;
}

/* Call LABEL.compute_semantic(SEM_VAL), convert the result implicitly to
   evaluation_semantic, and constant-evaluate it.  Returns NULL_TREE if the
   label has no compute_semantic facet (build_compute_semantic_call), and
   error_mark_node if the converted call does not evaluate to a constant.
   Used by compute_semantic_core.  */

static tree
call_label_method (tree label, uint16_t sem_val, location_t loc)
{
  tree conv = build_compute_semantic_call (label, sem_val);
  if (!conv)
    return NULL_TREE;
  return constant_facet_value (conv, TREE_TYPE (label), "compute_semantic",
			       loc);
}

/* Read the cached constexpr callee-side semantic.  Valid only after
   ensure_evaluation_semantic(contract, fndecl, true) has been called.  */

contract_evaluation_semantic
get_constexpr_evaluation_semantic (const_tree contract)
{
  tree s = CONTRACT_CONSTEXPR_EVALUATION_SEMANTIC (contract);
  gcc_checking_assert (s != NULL_TREE);
  return (contract_evaluation_semantic) tree_to_uhwi (s);
}

/* The gated base set: the four C++26 semantics, plus P3100 "assume" only
   when -fcontracts-allow-assume is in effect, the P4298 noexcept variants
   only under -fcontracts-p4298, and the P4405 prove and diagnose only under
   -fcontracts-p4405.  make_contract_query intersects the
   (flag-independent) label restriction with this set, so a gated semantic can
   never be present -- from a label or otherwise -- when its flag is off.  */

static inline uint16_t
contract_base_allowed_mask ()
{
  uint16_t mask = CES_ALL_ALLOWED;
  if (flag_contracts_allow_assume)
    mask |= (1 << CES_ASSUME);
  if (flag_contracts_p4298)
    mask |= (1 << CES_NOEXCEPT_ENFORCE) | (1 << CES_NOEXCEPT_OBSERVE);
  if (flag_contracts_p4405)
    mask |= (1 << CES_PROVE) | (1 << CES_DIAGNOSE);
  return mask;
}

/* Reconstruct a base, callee-side, non-constant-evaluation contract_query
   from the stored AST fields.  */

static contract_query
make_contract_query (tree contract, tree fndecl)
{
  contract_query q;
  q.fndecl = fndecl;
  q.loc = EXPR_LOCATION (contract);

  if (TREE_CODE (contract) == PRECONDITION_STMT)
    q.kind = CAK_PRE;
  else if (POSTCONDITION_P (contract))
    q.kind = CAK_POST;
  else if (TREE_CODE (contract) == ASSERTION_STMT)
    q.kind = CAK_ASSERT;
  else
    q.kind = CAK_INVALID;

  /* CONTRACT_ALLOWED_MASK holds the flag-independent label restriction
     (the label's allowed_semantics facet intersected with the full semantic
     set), or NULL_TREE for no restriction.  The -fcontracts-allow-assume gate
     is applied here, at query construction, by intersecting with the gated
     base set -- so it applies uniformly to every contract (including template
     instantiations) regardless of where the mask was computed.  */
  tree mask_tree = CONTRACT_ALLOWED_MASK (contract);
  uint16_t label_mask = mask_tree
    ? (uint16_t) tree_to_uhwi (mask_tree)
    : (uint16_t) CES_ALL_ALLOWED_WITH_EXTENSIONS;
  q.allowed_mask = label_mask & contract_base_allowed_mask ();

  q.groups = NULL;
  return q;
}

/* P3100: resolve the evaluation semantic for a synthesized implicit contract
   assertion guarding a core-language undefined behaviour, identified by UB_ID
   (the P3100 curly-brace identifier, used verbatim as the configuration
   group).  FNDECL is the enclosing function and LOC the site location.

   The builtin configuration (see contract_config_init) defaults implicit
   contract assertions to the "assume" semantic -- today's behaviour: no check
   is emitted and the UB is preserved.  A user configuration may select any
   other semantic for a given group; the caller is responsible for honouring
   the returned semantic.

   ALLOWED is the set of semantics the check supports.  "assume" is added to
   it always -- implicit "assume" is not gated on -fcontracts-allow-assume,
   since it introduces no new UB (it is the status quo) -- and the P4298
   noexcept variants under -fcontracts-p4298, but only when ALLOWED already
   permits a checking semantic.  */

contract_evaluation_semantic
resolve_implicit_contract_semantic (tree fndecl, location_t loc,
				    const char *ub_id, uint16_t allowed)
{
  auto_vec<const char *> groups;
  groups.safe_push (ub_id);

  /* ALLOWED is the base set this check supports; add "assume" always and the
     P4298 noexcept variants under the flag.  A configured semantic outside this
     set is clamped by contract_config_resolve via the fallback order.  The
     noexcept variants are the non-throwing counterparts of a *checking*
     semantic, so only widen with them when ALLOWED already permits some
     checking semantic -- otherwise a check that supports no checking at all
     (e.g. an opaque [[assume]], ALLOWED == {ignore}) would have a handler
     semantic re-admitted under -fcontracts-p4298 and evaluate a predicate it
     must not.  */
  uint16_t mask = allowed | (1 << CES_ASSUME);
  const uint16_t checking_mask
    = (1 << CES_OBSERVE) | (1 << CES_ENFORCE) | (1 << CES_QUICK);
  if (flag_contracts_p4298 && (allowed & checking_mask))
    mask |= (1 << CES_NOEXCEPT_ENFORCE) | (1 << CES_NOEXCEPT_OBSERVE);

  contract_query q;
  q.fndecl = fndecl;
  q.kind = CAK_IMPLICIT;
  q.allowed_mask = mask;
  q.groups = &groups;
  q.loc = loc;

  contract_config_result r = contract_config_resolve (&q);

  /* CES_INVALID means the allowed set admitted no semantic (e.g. a label whose
     allowed_semantics facet excludes everything the check supports).  That is
     an ill-formed configuration -- diagnose it rather than silently picking
     one.  */
  if (r.semantic == CES_INVALID)
    {
      error_at (loc, "no valid evaluation semantic for the implicit contract "
		     "assertion for %qs", ub_id);
      return CES_ASSUME;
    }
  return r.semantic;
}

/* Per-UB policy for the middle-end implicit checks (those instrumented by the
   ubsan pass), keyed by config group.  ALLOWED is the base set of C++26
   semantics the check can emit; all middle-end checks exclude the
   potentially-throwing "enforce"/"observe" (their site runs after EH lowering,
   so a throwing handler could not unwind) -- a configured throwing enf/obs is
   clamped by the best-fit fallback to the noexcept variant (under
   -fcontracts-p4298) or quick_enforce.  DEFINED_EB is true when the UB has a
   defined erroneous-behavior substitute, so that "ignore" instruments to
   produce that defined value (IMPLICIT_UB_DEFINED) rather than being a no-op:
   e.g. signed overflow coerces to the wrapped result, whereas a null
   dereference has no defined lvalue (ignore = raw operation).  COMMENT is the
   contract_violation comment for the reported violation.  */

struct implicit_ub_info
{
  uint16_t allowed;
  bool defined_eb;
  const char *comment;
};

static bool
implicit_ub_group_info (const char *group, implicit_ub_info *out)
{
  static const uint16_t mid_end_allowed
    = CES_ALL_ALLOWED & ~((1 << CES_ENFORCE) | (1 << CES_OBSERVE));
  /* The null-dereference check reports one of two leaves of
     ub:expr.unary.dereference.nullptr: .access at a load or store, and
     .indirection where the indirection is not followed by an access (a
     reference binding, a member call, the address of a member).  */
  if (strcmp (group, "ub:expr.unary.dereference.nullptr.access") == 0
      || strcmp (group, "ub:expr.unary.dereference.nullptr.indirection") == 0)
    {
      *out = { mid_end_allowed, false, "null pointer dereference" };
      return true;
    }
  /* The signed-overflow check reports .add, .sub, .mul or .negate of
     ub:expr.expr.eval.signed.integer by its operation, and the invalid
     value check .bool or .enum of ub:conv.lval.valid.representation by the
     type loaded.  */
  if (strcmp (group, "ub:expr.expr.eval.signed.integer.add") == 0
      || strcmp (group, "ub:expr.expr.eval.signed.integer.sub") == 0
      || strcmp (group, "ub:expr.expr.eval.signed.integer.mul") == 0
      || strcmp (group, "ub:expr.expr.eval.signed.integer.negate") == 0)
    {
      *out = { mid_end_allowed, true, "signed integer overflow" };
      return true;
    }
  if (strcmp (group, "ub:conv.lval.valid.representation.bool") == 0
      || strcmp (group, "ub:conv.lval.valid.representation.enum") == 0)
    {
      *out = { mid_end_allowed, true, "invalid value for its type" };
      return true;
    }
  if (strcmp (group, "ub:basic.align.object.alignment.store") == 0)
    {
      /* A misaligned access is an lvalue with no defined substitute, so ignore
	 is the raw access (like null-deref), not a coerced value.  This is
	 the .store leaf: the check is at a typed access, which a creation
	 performing no store (.nostore) never makes.  */
      *out = { mid_end_allowed, false, "misaligned pointer access" };
      return true;
    }
  return false;
}

/* LANG_HOOKS_RESOLVE_IMPLICIT_UB_SEMANTIC: called from the language-neutral
   middle end (the ubsan instrumentation pass) at a core-language UB site to
   learn how a P3100 implicit contract assertion for GROUP should react there.
   FNDECL is the function containing the site (cfun->decl at that point) and LOC
   is the site location; both feed the P3595 configuration query so that
   per-namespace and per-file/line matching work at the actual site.  Maps the
   resolved evaluation semantic onto the neutral enum implicit_ub_reaction
   (see gcc/ubsan.h), consulting the per-UB policy for the group.  */

int
cp_resolve_implicit_ub_semantic (tree fndecl, location_t loc, const char *group)
{
  implicit_ub_info info;
  if (!flag_contracts_p3100 || fndecl == NULL_TREE
      || !implicit_ub_group_info (group, &info))
    return IMPLICIT_UB_NONE;

  contract_evaluation_semantic sem
    = resolve_implicit_contract_semantic (fndecl, loc, group, info.allowed);

  switch (sem)
    {
    case CES_QUICK:
      return IMPLICIT_UB_TRAP;
    case CES_NOEXCEPT_ENFORCE:
      /* Non-throwing handler: the entry point cannot propagate an exception, so
	 no EH region is needed at the middle-end site.  The enforce/observe
	 distinction is preserved here so it survives inlining -- this single
	 mapping point feeds the reaction operand carried on every instrumented
	 site, and the handler-building langhook uses it directly.  */
      return IMPLICIT_UB_NOEXCEPT_ENFORCE;
    case CES_NOEXCEPT_OBSERVE:
      return IMPLICIT_UB_NOEXCEPT_OBSERVE;
    case CES_PROVE:
    case CES_DIAGNOSE:
      /* Never resolved: prove and diagnose are not among the semantics an
	 implicit contract assertion allows, so the best fit takes them to
	 ignore.  Should one get here, it behaves as ignore.  */
      gcc_checking_assert (false);
      /* FALLTHRU */
    case CES_IGNORE:
      /* Produce the defined erroneous value where one exists (e.g. the wrapped
	 result for signed overflow); otherwise no instrumentation.  */
      return info.defined_eb ? IMPLICIT_UB_DEFINED : IMPLICIT_UB_NONE;
    default:
      /* assume (and any unreachable throwing enf/obs): no instrumentation.  */
      return IMPLICIT_UB_NONE;
    }
}

static char *read_constant_char_array (tree);

/* Lazily extract group names from the label's group_names static constexpr
   member and cache as a TREE_LIST of STRING_CSTs in CONTRACT_GROUPS.
   Uses error_mark_node as sentinel for "checked, no groups".  */

static void
ensure_contract_groups (tree contract)
{
  if (CONTRACT_GROUPS (contract) != NULL_TREE)
    return;

  tree label = CONTRACT_LABEL (contract);
  if (!label || label == error_mark_node
      || !TREE_TYPE (label)
      || !CLASS_TYPE_P (TREE_TYPE (label)))
    {
      CONTRACT_GROUPS (contract) = error_mark_node;
      return;
    }

  tree label_type = TREE_TYPE (label);
  tree gn_member = lookup_member (label_type,
				  get_identifier ("group_names"),
				  /*protect=*/0, /*want_type=*/false,
				  tf_none);
  /* The concept requires `__is_const (decltype (t.group_names))'.  A
     `static constexpr' member is const-qualified already, and so is an array
     of const elements; a non-const member is not a facet.  P3400 requires
     const so that nothing implies a label's group membership could change at
     run time and have an effect -- nothing reads it after translation.  */
  tree gn_decl = (gn_member && gn_member != error_mark_node
		  && BASELINK_P (gn_member)
		  ? BASELINK_FUNCTIONS (gn_member) : gn_member);
  if (!gn_member || gn_member == error_mark_node
      || !label_facet_accessible_p (label_type, gn_member)
      || !gn_decl || !DECL_P (gn_decl)
      || !CP_TYPE_CONST_P (TREE_TYPE (gn_decl)))
    {
      CONTRACT_GROUPS (contract) = error_mark_node;
      return;
    }

  tree gn_val = finish_class_member_access_expr
    (label, get_identifier ("group_names"), false, tf_none);
  if (!gn_val || gn_val == error_mark_node)
    {
      CONTRACT_GROUPS (contract) = error_mark_node;
      return;
    }

  /* Read the names as the concept does ([support.contract.control.ident]):
     group_names[i] converted to const char*, for each of the
     sizeof (group_names) / sizeof (group_names[0]) elements -- an array of
     strings, of pointers to strings, or a std::array of either.  */
  location_t loc = EXPR_LOC_OR_LOC (label, input_location);
  tree elt0 = grok_array_decl (loc, gn_val, size_zero_node, NULL, tf_none);
  if (!elt0 || elt0 == error_mark_node)
    {
      CONTRACT_GROUPS (contract) = error_mark_node;
      return;
    }
  tree total = cxx_sizeof_or_alignof_type (loc, TREE_TYPE (gn_val),
					   SIZEOF_EXPR, false, false);
  tree each = cxx_sizeof_or_alignof_type (loc, TREE_TYPE (elt0),
					  SIZEOF_EXPR, false, false);
  if (!tree_fits_uhwi_p (total) || !tree_fits_uhwi_p (each)
      || integer_zerop (each))
    {
      CONTRACT_GROUPS (contract) = error_mark_node;
      return;
    }
  unsigned HOST_WIDE_INT count = tree_to_uhwi (total) / tree_to_uhwi (each);

  tree groups_list = NULL_TREE;
  for (unsigned HOST_WIDE_INT i = 0; i < count; ++i)
    {
      tree elt = grok_array_decl (loc, gn_val, size_int (i), NULL, tf_none);
      if (elt == error_mark_node)
	continue;
      tree conv = perform_implicit_conversion (const_string_type_node,
					       unshare_expr (elt), tf_none);
      if (conv == error_mark_node)
	continue;
      tree ptr = cxx_constant_value (conv, NULL_TREE, tf_none);
      const char *str = (ptr && ptr != error_mark_node
			 ? c_getstr (ptr) : NULL);
      /* A pointer into a character array inside a constant object -- the
	 library's own "name"group label keeps its names so -- is more than
	 c_getstr reads; read the array's value instead.  */
      char *buf = NULL;
      if (!str && TREE_CODE (TREE_TYPE (elt)) == ARRAY_TYPE)
	{
	  tree val = cxx_constant_value (elt, NULL_TREE, tf_none);
	  if (val && TREE_CODE (val) == STRING_CST)
	    str = TREE_STRING_POINTER (val);
	  else if (val && TREE_CODE (val) == CONSTRUCTOR)
	    {
	      buf = read_constant_char_array (val);
	      str = buf;
	    }
	}
      if (str && str[0] != '\0')
	groups_list = tree_cons (NULL_TREE,
				 build_string (strlen (str) + 1, str),
				 groups_list);
      XDELETEVEC (buf);
    }

  CONTRACT_GROUPS (contract) = groups_list
    ? nreverse (groups_list) : error_mark_node;
}

/* VAL is the constant value of a character array, a CONSTRUCTOR.  Return
   its contents as a NUL-terminated string allocated with XNEWVEC, or NULL
   if an element is not a byte we can read.  Elements the CONSTRUCTOR omits
   are zero; its elements may be indexed or ranges, and a run of them may be
   a single RAW_DATA_CST.  */

static char *
read_constant_char_array (tree val)
{
  tree type = TREE_TYPE (val);
  tree domain = TYPE_DOMAIN (type);
  if (!domain || !tree_fits_uhwi_p (TYPE_MAX_VALUE (domain)))
    return NULL;
  unsigned HOST_WIDE_INT len = tree_to_uhwi (TYPE_MAX_VALUE (domain)) + 1;
  char *buf = XCNEWVEC (char, len + 1);
  unsigned HOST_WIDE_INT pos = 0;
  unsigned k;
  tree idx, ch;
  FOR_EACH_CONSTRUCTOR_ELT (CONSTRUCTOR_ELTS (val), k, idx, ch)
    {
      unsigned HOST_WIDE_INT lo = pos, hi = pos;
      if (idx && TREE_CODE (idx) == RANGE_EXPR)
	{
	  if (!tree_fits_uhwi_p (TREE_OPERAND (idx, 0))
	      || !tree_fits_uhwi_p (TREE_OPERAND (idx, 1)))
	    goto fail;
	  lo = tree_to_uhwi (TREE_OPERAND (idx, 0));
	  hi = tree_to_uhwi (TREE_OPERAND (idx, 1));
	}
      else if (idx)
	{
	  if (!tree_fits_uhwi_p (idx))
	    goto fail;
	  lo = hi = tree_to_uhwi (idx);
	}
      if (TREE_CODE (ch) == RAW_DATA_CST)
	{
	  unsigned HOST_WIDE_INT n = RAW_DATA_LENGTH (ch);
	  if (lo != hi || lo + n > len)
	    goto fail;
	  memcpy (buf + lo, RAW_DATA_POINTER (ch), n);
	  pos = lo + n;
	  continue;
	}
      if (TREE_CODE (ch) != INTEGER_CST || hi >= len || lo > hi)
	goto fail;
      for (unsigned HOST_WIDE_INT p = lo; p <= hi; ++p)
	buf[p] = (char) TREE_INT_CST_LOW (ch);
      pos = hi + 1;
    }
  return buf;

 fail:
  XDELETEVEC (buf);
  return NULL;
}

/* Populate the groups vec from the contract's cached group names.  */

static void
fill_query_groups (contract_query *q, tree contract,
		   auto_vec<const char *> &vec)
{
  ensure_contract_groups (contract);
  tree groups = CONTRACT_GROUPS (contract);
  if (groups == error_mark_node)
    return;
  for (tree g = groups; g; g = TREE_CHAIN (g))
    vec.safe_push (TREE_STRING_POINTER (TREE_VALUE (g)));
  if (!vec.is_empty ())
    q->groups = &vec;
}

/* Does LABEL have a compute_semantic P3400 facet?  Used by the P3595 dynamic-
   dispatch path to decide whether a selector's value needs the label's
   transform (contract_label_transforms_p).  The test is call_label_method's,
   short of constant-evaluating the call: the facet must be callable and
   accessible, and its result implicitly convertible to evaluation_semantic.
   Overload resolution does not depend on the semantic's value, so any one
   will do.  */

static bool
label_has_compute_semantic (tree label)
{
  return build_compute_semantic_call (label, CES_ENFORCE) != NULL_TREE;
}

/* Core of the compute_semantic P3400 facet: apply LABEL's compute_semantic to
   SEM, if present.  Sets *IN_ALLOWED to whether the (possibly transformed)
   result lies within ALLOWED_MASK.  Returns the raw computed value when the
   facet is present, else SEM unchanged (with *IN_ALLOWED true).  */

static uint16_t
compute_semantic_core (tree label, uint16_t sem, uint16_t allowed_mask,
		       bool *in_allowed, location_t loc)
{
  *in_allowed = true;
  if (!label || label == error_mark_node)
    return sem;
  tree cs_result = call_label_method (label, sem, loc);
  if (cs_result && TREE_CODE (cs_result) == INTEGER_CST)
    {
      /* A value that is not a semantic is never in the allowed set.  */
      unsigned HOST_WIDE_INT computed
	= tree_fits_uhwi_p (cs_result) ? tree_to_uhwi (cs_result) : 0;
      *in_allowed = (computed > CES_INVALID && computed <= CES_LAST
		     && (allowed_mask & (1 << computed)) != 0);
      return *in_allowed ? (uint16_t) computed : sem;
    }
  return sem;
}

/* The contracts whose compute_semantic result has been diagnosed as outside
   the allowed set.  The runtime and the constant-evaluation semantic are
   resolved separately, and both can land on the same rejected result: the
   assertion is diagnosed once.  */

static GTY(()) hash_set<tree> *compute_semantic_diagnosed;

/* Apply the compute_semantic P3400 facet of LABEL to SEM for the P3595
   dynamic-dispatch path: when the compute_semantic result lands outside the
   allowed set ALLOWED_MASK, return CES_INVALID (the sentinel that stage 2
   turns into a runtime enforced violation) instead of issuing a
   compile-time error.  */

static uint16_t
apply_compute_semantic_value (tree label, uint16_t sem, uint16_t allowed_mask,
			      location_t loc)
{
  bool in_allowed;
  uint16_t computed = compute_semantic_core (label, sem, allowed_mask,
					     &in_allowed, loc);
  return in_allowed ? computed : (uint16_t) CES_INVALID;
}

/* D4405: LABEL.analysis_semantic(), implicitly converted to
   std::contracts::evaluation_semantic, or NULL_TREE if LABEL's type does not
   model analysis_semantic_label: the member does not exist, cannot be called
   without arguments, is inaccessible, or its result is not implicitly
   convertible to evaluation_semantic.  */

static tree
build_analysis_semantic_call (tree label)
{
  if (!label || label == error_mark_node
      || !TREE_TYPE (label) || !CLASS_TYPE_P (TREE_TYPE (label))
      || type_dependent_expression_p (label))
    return NULL_TREE;
  tree label_type = TREE_TYPE (label);
  tree fn = lookup_member (label_type, get_identifier ("analysis_semantic"),
			   /*protect=*/0, /*want_type=*/false, tf_none);
  if (!fn || fn == error_mark_node)
    return NULL_TREE;
  tree sem_type
    = find_std_contracts_type (get_identifier ("evaluation_semantic"));
  if (!sem_type || TREE_CODE (sem_type) != ENUMERAL_TYPE)
    return NULL_TREE;
  vec<tree, va_gc> *args = make_tree_vector ();
  /* Access is judged on the member chosen, as for compute_semantic.  */
  push_deferring_access_checks (dk_no_check);
  tree call = build_new_method_call (label_facet_object (label), fn, &args,
				     NULL_TREE, LOOKUP_NORMAL, NULL, tf_none);
  pop_deferring_access_checks ();
  release_tree_vector (args);
  if (!call || call == error_mark_node)
    return NULL_TREE;
  tree callee = cp_get_callee_fndecl_nofold (extract_call_expr (call));
  if (!callee || !label_facet_accessible_p (label_type, callee))
    return NULL_TREE;
  /* The concept's { t.analysis_semantic() } -> convertible_to<...>.  */
  if (!is_convertible (TREE_TYPE (call), sem_type))
    return NULL_TREE;
  tree conv = perform_implicit_conversion (sem_type, call, tf_none);
  if (!conv || conv == error_mark_node)
    return NULL_TREE;
  return conv;
}

/* D4405: the semantic CONTRACT's assertion-control object gives the analysis
   evaluation through its analysis_semantic facet ("P3400R5 Analysis
   Evaluation Configuration"), or ignore when it has none.  The facet must be
   a constant expression whose value is ignore, diagnose or prove; otherwise
   the program is ill-formed, diagnosed at LOC, and ignore is returned.  */

static contract_evaluation_semantic
label_analysis_semantic (tree contract, location_t loc)
{
  tree label = CONTRACT_LABEL (contract);
  tree conv = build_analysis_semantic_call (label);
  if (!conv)
    return CES_IGNORE;
  tree val = constant_facet_value (conv, TREE_TYPE (label),
				   "analysis_semantic", loc);
  if (val == error_mark_node)
    return CES_IGNORE;
  if (TREE_CODE (val) == INTEGER_CST && tree_fits_uhwi_p (val))
    {
      unsigned HOST_WIDE_INT v = tree_to_uhwi (val);
      if (v == CES_IGNORE || v == CES_PROVE || v == CES_DIAGNOSE)
	return (contract_evaluation_semantic) v;
    }
  error_at (loc, "%<analysis_semantic%> result is not %<ignore%>, "
	    "%<diagnose%> or %<prove%>");
  return CES_IGNORE;
}

/* D4405, step 6 of "Adjusting to the Allowed Semantics": apply CONTRACT's
   compute_semantic facet, if any, to *RUNTIME, the adjusted runtime
   semantic of the evaluation, whose allowed set is RUNTIME_MASK; *ANALYSIS
   is the adjusted semantic of the analysis evaluation, whose allowed set is
   ANALYSIS_MASK.  A result of prove or diagnose moves to the analysis
   evaluation, which takes the stronger and must allow it; the runtime
   evaluation then becomes ignore, which, like any other result, must be in
   RUNTIME_MASK.  A constant evaluation (IN_CE) has no analysis evaluation:
   RUNTIME_MASK then adds the whole allowed set to the runtime one, and a
   prove or diagnose in it is used as enforce or observe
   (contract_constant_evaluation_semantic, DECISIONS.md E142).  Otherwise the program is ill-formed (diagnosed once
   per contract), and both are left as they were.  */

static void
apply_compute_semantic_split (tree contract,
			      contract_evaluation_semantic *runtime,
			      contract_evaluation_semantic *analysis,
			      uint16_t runtime_mask, uint16_t analysis_mask,
			      bool in_ce, location_t loc)
{
  tree label = CONTRACT_LABEL (contract);
  if (!label || label == error_mark_node)
    return;
  tree cs_result = call_label_method (label, *runtime, loc);
  if (!cs_result || TREE_CODE (cs_result) != INTEGER_CST)
    return;
  unsigned HOST_WIDE_INT computed
    = tree_fits_uhwi_p (cs_result) ? tree_to_uhwi (cs_result) : 0;
  bool ok;
  if (in_ce)
    {
      ok = (computed > CES_INVALID && computed <= CES_LAST
	    && (runtime_mask & (1 << computed)) != 0);
      if (ok)
	*runtime = contract_constant_evaluation_semantic
		     ((contract_evaluation_semantic) computed);
    }
  else if (computed == CES_PROVE || computed == CES_DIAGNOSE)
    {
      contract_evaluation_semantic a
	= contract_analysis_stronger (*analysis,
				      (contract_evaluation_semantic) computed);
      ok = ((analysis_mask & (1 << a)) != 0
	    && (runtime_mask & (1 << CES_IGNORE)) != 0);
      if (ok)
	{
	  *analysis = a;
	  *runtime = CES_IGNORE;
	}
    }
  else
    {
      /* A value that is not a semantic is never in the allowed set.  */
      ok = (computed > CES_INVALID && computed <= CES_LAST
	    && (runtime_mask & (1 << computed)) != 0);
      if (ok)
	*runtime = (contract_evaluation_semantic) computed;
    }
  if (ok)
    return;
  if (!compute_semantic_diagnosed)
    compute_semantic_diagnosed = hash_set<tree>::create_ggc (8);
  if (!compute_semantic_diagnosed->add (contract))
    error_at (loc, "%<compute_semantic%> result is not in the "
	      "allowed evaluation semantics");
}

/* D4405: the runtime callee-side semantic slot holds the runtime semantic in
   its low byte and the semantic of the analysis evaluation above it.  A slot
   written with a bare semantic (a non-virtual wrapper's copy, holding the
   caller-side semantic) has an analysis of 0, read as ignore.  */

static tree
build_contract_semantic_slot (contract_evaluation_semantic runtime,
			      contract_evaluation_semantic analysis)
{
  return build_int_cst (uint16_type_node,
			(unsigned) runtime | ((unsigned) analysis << 8));
}

static void
read_contract_semantic_slot (tree slot, contract_evaluation_semantic *runtime,
			     contract_evaluation_semantic *analysis)
{
  unsigned HOST_WIDE_INT v = tree_to_uhwi (slot);
  *runtime = (contract_evaluation_semantic) (v & 0xff);
  unsigned a = (v >> 8) & 0xff;
  *analysis = a ? (contract_evaluation_semantic) a : CES_IGNORE;
}

/* Lazily resolve the callee-side semantic for CONTRACT.  Uses the runtime
   slot when !IN_CE, the constexpr slot when IN_CE.  Caches the result.
   D4405: the result is the semantic of the runtime evaluation, never prove
   or diagnose; the runtime slot also carries the semantic of the analysis
   evaluation (ensure_analysis_semantic).  */

contract_evaluation_semantic
ensure_evaluation_semantic (tree contract, tree fndecl, bool in_ce)
{
  tree *slot = in_ce
    ? &CONTRACT_CONSTEXPR_EVALUATION_SEMANTIC (contract)
    : &CONTRACT_EVALUATION_SEMANTIC (contract);

  if (*slot != NULL_TREE)
    {
      if (in_ce)
	return (contract_evaluation_semantic) tree_to_uhwi (*slot);
      contract_evaluation_semantic runtime, analysis;
      read_contract_semantic_slot (*slot, &runtime, &analysis);
      return runtime;
    }

  contract_query q = make_contract_query (contract, fndecl);
  q.caller_side = false;
  q.in_constant_evaluation = in_ce;
  /* D4405, step 3: the label's analysis_semantic facet.  */
  if (!in_ce)
    q.label_analysis = label_analysis_semantic (contract,
						EXPR_LOCATION (contract));

  auto_vec<const char *> groups_vec;
  fill_query_groups (&q, contract, groups_vec);

  contract_config_result res = contract_config_resolve (&q);
  contract_evaluation_semantic runtime = res.runtime;
  contract_evaluation_semantic analysis = res.analysis;
  bool dynamic_no_default = (!in_ce && res.dyn_name && res.no_static_default);
  if (res.semantic == CES_INVALID && !dynamic_no_default)
    {
      error_at (EXPR_LOCATION (contract),
		"no valid evaluation semantic for contract assertion");
      runtime = in_ce ? CES_OBSERVE : CES_ENFORCE;
      analysis = CES_IGNORE;
    }

  if (dynamic_no_default)
    {
      /* P3595: the entry asked for a dynamic selector with no compile-time
	 default, which the config parser accepts deliberately -- the user
	 supplies the selector themselves and provideweak has already been
	 forced false, so no weak definition needs a value to return.  The
	 cached semantic is therefore never used: the descriptor keeps the
	 contract active and the selector decides at run time.  Mirror what
	 resolve_caller_semantic already does for the identical config, and
	 do not run compute_semantic over a placeholder -- the dynamic path
	 applies transform_semantic to the selector's own result instead.  */
      runtime = CES_IGNORE;
    }
  else
    /* D4405, step 6.  */
    apply_compute_semantic_split
      (contract, &runtime, &analysis,
       ((in_ce ? q.allowed_mask : 0)
	| contract_runtime_allowed_mask (q.allowed_mask, false)),
       contract_analysis_allowed_mask (q.allowed_mask), in_ce,
       EXPR_LOCATION (contract));

  if (in_ce)
    *slot = build_int_cst (uint16_type_node, runtime);
  else
    *slot = build_contract_semantic_slot (runtime, analysis);

  /* Cache the runtime dynamic-selector descriptor, if any.  A dynamic
     descriptor only exists for the runtime slot (!in_ce); constant
     evaluation never has one (P3595 spec 3, enforced in
     contract_config_resolve).  Store the name as an IDENTIFIER_NODE and
     pack the configured semantic, linkage and provideweak into an
     INTEGER_CST so the whole thing is GC-safe.  */
  if (!in_ce && res.dyn_name)
    {
      unsigned HOST_WIDE_INT packed
	= ((unsigned HOST_WIDE_INT) res.configured << 8)
	  | ((unsigned HOST_WIDE_INT) res.dyn_linkage << 1)
	  | (res.dyn_provideweak ? 1 : 0);
      CONTRACT_DYNAMIC (contract)
	= build_tree_list (get_identifier (res.dyn_name),
			   build_int_cst (uint16_type_node, packed));
    }

  return runtime;
}

/* D4405: the semantic of CONTRACT's analysis evaluation (ignore, diagnose or
   prove) and, in *RUNTIME if non-null, that of its callee-side runtime
   evaluation, both resolved for FNDECL if not yet
   (ensure_evaluation_semantic).  */

contract_evaluation_semantic
ensure_analysis_semantic (tree contract, tree fndecl,
			  contract_evaluation_semantic *runtime)
{
  ensure_evaluation_semantic (contract, fndecl, /*in_ce=*/false);
  contract_evaluation_semantic r, a;
  read_contract_semantic_slot (CONTRACT_EVALUATION_SEMANTIC (contract), &r,
			       &a);
  if (runtime)
    *runtime = r;
  return a;
}

/* The result of resolving a contract's caller-side semantic for a specific
   call site: the clamped, compute_semantic-applied SEMANTIC of the
   caller-side runtime evaluation, plus the P3595 dynamic-selector
   descriptor (DYN_NAME == NULL when the resolution is not dynamic), whose
   DYN_CONFIGURED is the entry's own semantic.  D4405: ANALYSIS is what
   this resolution gives the analysis evaluation at the call (a configured
   caller-side prove or diagnose, DECISIONS.md E142), ignore otherwise.  */

struct caller_resolution
{
  contract_evaluation_semantic semantic;
  contract_evaluation_semantic analysis;
  const char *dyn_name;
  unsigned char dyn_linkage;
  bool dyn_provideweak;
  contract_evaluation_semantic dyn_configured;
};

/* Resolve the caller-side semantic for CONTRACT for a specific call site
   described by CALLER_LOC and CALLER_FNDECL.  This does not cache into
   any AST slot -- the result depends on the call site, so it must be
   recomputed per call.  */

static caller_resolution
resolve_caller_semantic (tree contract, tree fndecl,
			 location_t caller_loc, tree caller_fndecl)
{
  contract_query q = make_contract_query (contract, fndecl);
  q.caller_side = true;
  q.in_constant_evaluation = false;
  /* IGNORE, the caller-side opt-out default, is always in the allowed set
     of a caller-side evaluation (contract_runtime_allowed_mask).  */
  q.caller_loc = caller_loc;
  q.caller_fndecl = caller_fndecl;

  auto_vec<const char *> groups_vec;
  fill_query_groups (&q, contract, groups_vec);

  contract_config_result r = contract_config_resolve (&q);
  contract_evaluation_semantic runtime = r.runtime;
  contract_evaluation_semantic analysis = r.analysis;
  if (r.semantic == CES_INVALID)
    {
      runtime = CES_IGNORE;
      analysis = CES_IGNORE;
    }

  /* Apply the label's compute_semantic facet only when caller-side checking
     is actually engaged -- i.e. the resolved caller semantic emits a real
     check (observe/enforce/quick) -- never to the opt-out default
     (ignore/assume).  This preserves the caller-side opt-in model: a call
     site with no matching caller rule resolves to ignore and must stay
     ignore (no wrapper), so a label whose compute_semantic maps
     ignore->observe cannot resurrect a caller-side check that the call site
     never opted into.  Mirrors the callee-side path
     (ensure_evaluation_semantic), D4405's step 6, validating against the
     caller-side allowed sets.  */
  if (!contract_semantic_emits_no_check (runtime))
    apply_compute_semantic_split
      (contract, &runtime, &analysis,
       contract_runtime_allowed_mask (q.allowed_mask, true),
       contract_analysis_allowed_mask (q.allowed_mask), false,
       EXPR_LOCATION (contract));
  caller_resolution out;
  out.semantic = runtime;
  out.analysis = analysis;
  out.dyn_name = r.dyn_name;              /* NULL unless dynamic */
  out.dyn_linkage = r.dyn_linkage;
  out.dyn_provideweak = r.dyn_provideweak;
  out.dyn_configured = r.configured;
  return out;
}

/* Constexpr-semantic predicate helpers.  Valid after
   ensure_evaluation_semantic(contract, fndecl, true).  */

bool
contract_constexpr_ignored_p (const_tree contract)
{
  contract_evaluation_semantic s = get_constexpr_evaluation_semantic (contract);
  return s <= CES_IGNORE || contract_semantic_has_no_runtime_check (s);
}

bool
contract_constexpr_terminating_p (const_tree contract)
{
  contract_evaluation_semantic s = get_constexpr_evaluation_semantic (contract);
  return s == CES_ENFORCE || s == CES_QUICK || s == CES_NOEXCEPT_ENFORCE;
}

/* Get location of the last contract in CONTRACTS.  */

static location_t
get_contract_end_loc (tree contracts)
{
  gcc_checking_assert (contracts && TREE_VEC_LENGTH (contracts) > 0);
  tree last = TREE_VEC_ELT (contracts, TREE_VEC_LENGTH (contracts) - 1);
  return EXPR_LOCATION (last);
}

/* Build the contract specifiers for a function from CONTRACTS, which are in
   source order.  Returns NULL_TREE when there are none.  */

tree
build_contract_specifiers (vec<tree, va_gc> *contracts)
{
  unsigned len = vec_safe_length (contracts);
  if (!len)
    return NULL_TREE;

  tree specs = make_tree_vec (len);
  for (unsigned ix = 0; ix < len; ix++)
    TREE_VEC_ELT (specs, ix) = (*contracts)[ix];
  return specs;
}

/* Append the contract specifiers in SECOND to those in FIRST, either of
   which may be NULL_TREE.  Neither input is modified.  */

tree
contract_specifiers_concat (tree first, tree second)
{
  if (!first)
    return second;
  if (!second)
    return first;

  int flen = TREE_VEC_LENGTH (first);
  int slen = TREE_VEC_LENGTH (second);
  tree specs = make_tree_vec (flen + slen);
  for (int ix = 0; ix < flen; ix++)
    TREE_VEC_ELT (specs, ix) = TREE_VEC_ELT (first, ix);
  for (int ix = 0; ix < slen; ix++)
    TREE_VEC_ELT (specs, flen + ix) = TREE_VEC_ELT (second, ix);
  return specs;
}

struct GTY(()) contract_decl
{
  tree contract_specifiers;
  location_t note_loc;
};

/* Hash a key of the maps below by something a precompiled header preserves:
   writing one moves every tree, and a table hashed on the old addresses
   finds almost nothing when read back (PCH is not rehashed).  A decl or a
   type hashes by its uid, an identifier by its spelling; anything else --
   a contract statement, say, which never outlives the function it is
   in -- by its address.  */

struct contract_pch_hash : ggc_ptr_hash <tree_node>
{
  static hashval_t hash (tree t)
  {
    if (DECL_P (t))
      return DECL_UID (t);
    if (TYPE_P (t))
      return TYPE_UID (t);
    if (identifier_p (t))
      return IDENTIFIER_HASH_VALUE (t);
    return ggc_ptr_hash <tree_node>::hash (t);
  }
};

struct contract_decl_traits
  : simple_hashmap_traits <contract_pch_hash, contract_decl> {};
struct pch_traits
  : simple_hashmap_traits <contract_pch_hash, tree> {};

static GTY(()) hash_map<tree, contract_decl, contract_decl_traits>
  *contract_decl_map;

/* True if a use of `this' at the current point is in a contract predicate,
   where [expr.prim.this] combines const with the cv-qualifiers of its type --
   and so of the object it points to, whether reached explicitly or through an
   implicit member access.  */

bool
contract_this_is_const_p (void)
{
  return flag_contracts && processing_contract_condition;
}

/* Return PTR, a value of `this', as a pointer to const: the type `this' has
   within a contract predicate.  */

tree
contract_const_this (tree ptr)
{
  if (ptr == NULL_TREE || ptr == error_mark_node)
    return ptr;
  tree type = TREE_TYPE (ptr);
  if (!type || !TYPE_PTR_P (type) || CP_TYPE_CONST_P (TREE_TYPE (type)))
    return ptr;
  tree obj = TREE_TYPE (type);
  obj = cp_build_qualified_type (obj, cp_type_quals (obj) | TYPE_QUAL_CONST);
  return build_nop (build_pointer_type (obj), ptr);
}

/* Converts a contract condition to bool and ensures it has a location.  */

tree
finish_contract_condition (cp_expr condition)
{
  if (!condition || error_operand_p (condition))
    return condition;

  /* Ensure we have the condition location saved in case we later need to
     emit a conversion error during template instantiation and wouldn't
     otherwise have it.  This differs from maybe_wrap_with_location in that
     it allows wrappers on EXCEPTIONAL_CLASS_P which includes CONSTRUCTORs.  */
  if (!CAN_HAVE_LOCATION_P (condition)
      && condition.get_location () != UNKNOWN_LOCATION)
    {
      tree_code code
	= (((CONSTANT_CLASS_P (condition) && TREE_CODE (condition) != STRING_CST)
	    || (TREE_CODE (condition) == CONST_DECL && !TREE_STATIC (condition)))
	  ? NON_LVALUE_EXPR : VIEW_CONVERT_EXPR);
      condition = build1_loc (condition.get_location (), code,
			      TREE_TYPE (condition), condition);
      EXPR_LOCATION_WRAPPER_P (condition) = true;
    }

  /* A parameter pack named in the predicate must be expanded within it, as in
     any other expression.  */
  if (check_for_bare_parameter_packs (condition))
    return error_mark_node;

  if (type_dependent_expression_p (condition))
    return condition;

  return condition_conversion (condition);
}

/* True when TYPE is a reference type, or a pack expansion whose pattern is
   one.  A reference cannot be cv-qualified; cp_build_qualified_type recurses
   through a pack expansion into its pattern, so both spellings have to be
   recognized together.  */

static bool
contract_ref_or_ref_pack_p (tree type)
{
  if (!type || type == error_mark_node)
    return false;
  if (TREE_CODE (type) == TYPE_PACK_EXPANSION)
    type = PACK_EXPANSION_PATTERN (type);
  return type && type != error_mark_node && TYPE_REF_P (type);
}

/* Wrap the DECL into VIEW_CONVERT_EXPR representing const qualified version
   of the declaration.  */

tree
view_as_const (tree decl)
{
  if (decl
      && !CP_TYPE_CONST_P (TREE_TYPE (decl))
      /* A reference type cannot be cv-qualified, and there is nothing here to
	 constify: a reference is immutable already, and access *through* it is
	 constified where it is dereferenced, on the REFERENCE_REF below.

	 This is reachable only for a function parameter PACK of reference
	 type.  A non-pack reference parameter reaches this as an already
	 dereferenced REFERENCE_REF, whose type is the referent's, so it never
	 gets here.  A pack element is still a bare PARM_DECL at this point,
	 because the pack has not been expanded, and its type is a
	 TYPE_PACK_EXPANSION whose PATTERN is the reference -- which is why
	 testing the type itself is not enough: cp_build_qualified_type
	 recurses into the pattern, so the reference check fires in there and
	 the hard error ("'const' qualifiers cannot be applied to 'Args&'")
	 rejected a well-formed program.  PR c++/126878, and PR c++/126039 for
	 the `auto&&...' spelling of the same thing.

	 Leaving the pack alone is correct rather than merely quiet: at
	 instantiation each expanded element is an ordinary reference
	 parameter and is constified through the usual path.  */
      && !contract_ref_or_ref_pack_p (TREE_TYPE (decl)))
    {
      gcc_checking_assert (!contract_const_wrapper_p (decl));
      tree ctype = TREE_TYPE (decl);
      location_t loc =
	  EXPR_P (decl) ? EXPR_LOCATION (decl) : DECL_SOURCE_LOCATION (decl);
      ctype = cp_build_qualified_type (ctype, (cp_type_quals (ctype)
					       | TYPE_QUAL_CONST));
      decl = build1 (VIEW_CONVERT_EXPR, ctype, decl);
      SET_EXPR_LOCATION (decl, loc);
      /* Mark the VCE as contract const wrapper.  */
      CONST_WRAPPER_P (decl) = true;
    }
  return decl;
}

/* True if DECL is declared in one of the binding levels between the current
   one and CONTRACT_LEVEL, exclusive of CONTRACT_LEVEL itself -- that is,
   inside a lambda that appears in the predicate rather than outside the
   predicate.  [expr.prim.id.unqual]/3+d only constifies a variable "declared
   outside of C".  */

static bool
declared_inside_contract_predicate_p (tree decl,
				      cp_binding_level *contract_level)
{
  for (cp_binding_level *b = current_binding_level;
       b && b != contract_level;
       b = b->level_chain)
    for (tree t = b->names; t; t = DECL_CHAIN (t))
      if (t == decl)
	return true;
  return false;
}

/* True if DECL, named at the current point, must be constified because it is
   named from inside a LAMBDA appearing in a contract predicate.

   processing_contract_condition only tests whether the INNERMOST binding
   level is the contract scope, so it is false as soon as a lambda in the
   predicate pushes its own scopes; [expr.prim.id.unqual]/3+d still covers a
   variable named there.  The paragraph's own example is explicit about this:
   given a namespace-scope `int n`, `pre([=,&i,*this] mutable { ++n; ... }())`
   marks `++n` an error.  When the innermost level is the contract scope, the
   ordinary path handles the name and this is not reached.

   Two exemptions keep the rest of that example working:

     * A lambda capture proxy is not the variable; the id-expression denotes a
       member of the closure type, which the example marks OK (`++p`, `++r`).
       A captured entity that must be const already is -- the capture
       initializer was constified out in the enclosing predicate, which is why
       `++i` on a by-reference capture is an error for its own reason.
     * An entity declared inside the predicate -- a local of the lambda -- is
       not "declared outside of C" (`++j`, `++k` in the example).

   Members reached through a captured `*this` (`++this->z`, `++z`) never come
   here: they are FIELD_DECLs on the member-access path, not variables.  */

static bool
constify_from_lambda_in_predicate_p (tree decl)
{
  if (processing_contract_condition)
    return false;

  cp_binding_level *contract_level = NULL;
  for (cp_binding_level *b = current_binding_level; b; b = b->level_chain)
    {
      if (b->kind == sk_contract)
	{
	  contract_level = b;
	  break;
	}
      /* A contract scope is inside a function; reaching namespace scope means
	 there is none enclosing us.  */
      if (b->kind == sk_namespace)
	return false;
    }
  if (!contract_level)
    return false;

  /* Reach the underlying declaration: the caller may hand us a location
     wrapper, or a dereference of a reference (including a by-reference
     capture), and is_capture_proxy asserts on the former.  */
  tree d = decl;
  STRIP_ANY_LOCATION_WRAPPER (d);
  if (REFERENCE_REF_P (d))
    {
      d = TREE_OPERAND (d, 0);
      STRIP_ANY_LOCATION_WRAPPER (d);
    }
  if (!DECL_P (d))
    return false;

  if (is_capture_proxy (d))
    return false;

  return !declared_inside_contract_predicate_p (d, contract_level);
}

/* True if naming DECL at the current point is a use from inside a contract
   predicate that [expr.prim.id.unqual]/3+d constifies.  */

bool
constify_in_contract_predicate_p (tree decl)
{
  return (processing_contract_condition
	  || constify_from_lambda_in_predicate_p (decl));
}

/* Constify access to DECL from within the contract condition.

   P3098: Postcondition capture VAR_DECLs are exempt from const-ification.
   They are local to the assertion and the user may need to mutate them in
   the predicate (e.g., post [iter = x] (++iter == y)).  Captures are
   identified by VAR_P && DECL_ARTIFICIAL.  Note: this exemption may be
   revisited if the design changes -- captures could become const in a
   future revision.  */

tree
constify_contract_access (tree decl)
{
  if (!TREE_READONLY (decl)
      && (VAR_P (decl)
	  || (TREE_CODE (decl) == PARM_DECL)
	  || (REFERENCE_REF_P (decl)
	      && (VAR_P (TREE_OPERAND (decl, 0))
		  || (TREE_CODE (TREE_OPERAND (decl, 0)) == PARM_DECL)
		  || (TREE_CODE (TREE_OPERAND (decl, 0))
		      == TEMPLATE_PARM_INDEX)))))
    {
      /* P3098: Skip const-ification for postcondition capture variables.
	 Captures are VAR_DECLs marked DECL_ARTIFICIAL in the contract scope.
	 A lambda capture
	 proxy is an artificial VAR_DECL too, and is constified like the
	 entity it captures, by copy or by reference.  */
      if (VAR_P (decl) && DECL_ARTIFICIAL (decl) && !is_capture_proxy (decl))
	return decl;
      if (REFERENCE_REF_P (decl)
	  && VAR_P (TREE_OPERAND (decl, 0))
	  && DECL_ARTIFICIAL (TREE_OPERAND (decl, 0))
	  && !is_capture_proxy (TREE_OPERAND (decl, 0)))
	return decl;

      decl = view_as_const (decl);
    }

  return decl;
}

/* Indicate that PARM_DECL DECL is ODR used in a postcondition.  */

static void
set_parm_used_in_post (tree decl, bool constify = true)
{
  gcc_checking_assert (TREE_CODE (decl) == PARM_DECL);
  DECL_LANG_FLAG_4 (decl) = constify;
}

/* Test if PARM_DECL is ODR used in a postcondition.  */

static bool
parm_used_in_post_p (const_tree decl)
{
  /* Check if this parameter is odr used within a function's postcondition  */
  return ((TREE_CODE (decl) == PARM_DECL) && DECL_LANG_FLAG_4 (decl));
}

/* Data for check_postcondition_odr_use_r.  */

struct postcondition_odr_use_data
{
  tree fndecl;		/* The function whose postcondition this is.  */
  location_t loc;	/* Fallback location for the diagnostic.  */
  bool diagnosed;	/* An odr-use was diagnosed.  */
};

static tree check_postcondition_odr_use_r (tree *, int *, void *);

/* E is a discarded-value expression inside a postcondition predicate.  Its
   potential results ([basic.def.odr]/2) are not odr-uses, so a parameter that
   is one of them is exempt; everything else in E is walked normally, because
   a subexpression that is not a potential result -- an argument to a call, the
   condition of a ?: -- is evaluated and odr-uses whatever it names.  */

static void
walk_discarded_operand (tree e, postcondition_odr_use_data *d)
{
  if (!e)
    return;

  STRIP_ANY_LOCATION_WRAPPER (e);
  /* The contract const wrapper and plain lvalue wrappers are transparent.  A
     NOP_EXPR is deliberately not stripped: an ordinary conversion applies the
     lvalue-to-rvalue conversion, which makes its operand odr-used.  */
  while (TREE_CODE (e) == VIEW_CONVERT_EXPR
	 || TREE_CODE (e) == NON_LVALUE_EXPR)
    {
      e = TREE_OPERAND (e, 0);
      STRIP_ANY_LOCATION_WRAPPER (e);
    }

  /* A cast to void is itself a discarded-value expression, and convert_to_void
     also builds one when it does not fold the operand away.

     TREE_TYPE can be null here, so test it before asking what it is.  A
     postcondition's result binding is a PARM_DECL of type auto until the
     return type is deduced, which makes the predicate type-dependent while it
     is being parsed; a dependent member access, call, subscript or ?: is then
     built by build_min_nt, and those carry no type at all.  A null-typed tree
     is never a cast to void, so skipping this is also the right answer and
     not merely a safe one.  (Dependent unary and binary operators do get a
     dependent type, which is why `(r.id, true)` crashed here and
     `(r.id + 0, true)` did not.)  */
  if (TREE_TYPE (e)
      && VOID_TYPE_P (TREE_TYPE (e))
      && (TREE_CODE (e) == CONVERT_EXPR || TREE_CODE (e) == NOP_EXPR)
      && TREE_OPERAND_LENGTH (e) == 1)
    {
      walk_discarded_operand (TREE_OPERAND (e, 0), d);
      return;
    }
  /* In a template the cast to void is still the dependent CAST_EXPR (its
     operand a TREE_LIST of one expression) or STATIC_CAST_EXPR.  */
  if (TREE_TYPE (e) && VOID_TYPE_P (TREE_TYPE (e)))
    {
      if (TREE_CODE (e) == CAST_EXPR
	  && TREE_OPERAND (e, 0)
	  && TREE_CODE (TREE_OPERAND (e, 0)) == TREE_LIST
	  && !TREE_CHAIN (TREE_OPERAND (e, 0)))
	{
	  walk_discarded_operand (TREE_VALUE (TREE_OPERAND (e, 0)), d);
	  return;
	}
      if (TREE_CODE (e) == STATIC_CAST_EXPR)
	{
	  walk_discarded_operand (TREE_OPERAND (e, 0), d);
	  return;
	}
    }

  switch (TREE_CODE (e))
    {
    case PARM_DECL:
      /* A potential result, and not odr-used -- unless the lvalue-to-rvalue
	 conversion is applied after all, which for a discarded-value
	 expression happens only for a volatile glvalue ([expr.context]).  */
      if (!CP_TYPE_VOLATILE_P (TREE_TYPE (e)))
	return;
      break;

    case COMPOUND_EXPR:
      /* `(a, b)` discarded: b supplies the potential results, and a is itself
	 a discarded-value expression.  This is what `post ((x, y, true))`
	 needs, since that parses as `((x, y), true)`.  */
      walk_discarded_operand (TREE_OPERAND (e, 0), d);
      walk_discarded_operand (TREE_OPERAND (e, 1), d);
      return;

    case COND_EXPR:
      /* The second and third operands supply the potential results; the
	 condition is evaluated for its value.  */
      cp_walk_tree_without_duplicates (&TREE_OPERAND (e, 0),
				       check_postcondition_odr_use_r, d);
      walk_discarded_operand (TREE_OPERAND (e, 1), d);
      walk_discarded_operand (TREE_OPERAND (e, 2), d);
      return;

    case COMPONENT_REF:
      /* A class member access naming a non-static data member: its object
	 expression supplies the potential results ([basic.def.odr]/3), so
	 `(s.m, true)` does not odr-use s.  Unless the access is volatile,
	 when the lvalue-to-rvalue conversion is applied to it.  */
      if (TREE_TYPE (e)
	  && TREE_CODE (TREE_OPERAND (e, 1)) == FIELD_DECL
	  && !CP_TYPE_VOLATILE_P (TREE_TYPE (e)))
	{
	  walk_discarded_operand (TREE_OPERAND (e, 0), d);
	  return;
	}
      /* In a template, an access through a type-dependent object is built by
	 build_min_nt with the member still a name: whether it names a
	 non-static data member, and whether it is volatile, is not known until
	 substitution.  Take the object as a potential result for now; the
	 walk over the substituted predicate decides (GCC-663).  */
      if (TREE_CODE (TREE_OPERAND (e, 1)) != FIELD_DECL
	  && type_dependent_expression_p (TREE_OPERAND (e, 0)))
	{
	  walk_discarded_operand (TREE_OPERAND (e, 0), d);
	  return;
	}
      break;

    default:
      break;
    }

  cp_walk_tree_without_duplicates (&e, check_postcondition_odr_use_r, d);
}

/* cp_walk_tree callback implementing [dcl.contract.func]/7 over a FINISHED
   postcondition predicate: "If the predicate of a postcondition assertion of
   a function f odr-uses a non-reference parameter of f, that parameter ...
   shall have const type."  */

static tree
check_postcondition_odr_use_r (tree *tp, int *walk_subtrees, void *data)
{
  tree t = *tp;
  auto *d = (postcondition_odr_use_data *) data;

  /* An unevaluated operand is not an odr-use.  Most of these have already
     folded away by now, but a dependent one has not.

     The set to stop at is not a judgement call: cp_walk_subtrees enters
     exactly three kinds of subtree under `cp_unevaluated`, and this switch
     covers all three -- including a decltype whose operand is still
     dependent, and a requires-expression, which survives into the finished
     predicate whether or not anything in it is dependent.  */
  switch (TREE_CODE (t))
    {
    case SIZEOF_EXPR:
    case ALIGNOF_EXPR:
    case NOEXCEPT_EXPR:
    case AT_ENCODE_EXPR:
    case DECLTYPE_TYPE:
    case REQUIRES_EXPR:
      *walk_subtrees = 0;
      return NULL_TREE;

    /* An unexpanded pack, or a pack-index-expression whose index has not yet
       selected an element.  Substitution rewrites these to the element that
       is actually odr-used, and the walk that runs then sees only that one;
       descending here would check every element of the pack.  */
    case PACK_INDEX_EXPR:
      *walk_subtrees = 0;
      return NULL_TREE;

    /* A typeid survives unlowered only in a template.  Its operand is
       unevaluated unless it is a glvalue of polymorphic class type, which a
       dependent operand is not known to be; after substitution typeid is
       lowered, and the walk over the substituted predicate decides.  */
    case TYPEID_EXPR:
      {
	tree op = TREE_OPERAND (t, 0);
	tree type = TYPE_P (op) ? NULL_TREE : TREE_TYPE (op);
	if (!type || dependent_type_p (type)
	    || !(CLASS_TYPE_P (type) && TYPE_POLYMORPHIC_P (type)))
	  {
	    *walk_subtrees = 0;
	    return NULL_TREE;
	  }
	break;
      }

    case COMPOUND_EXPR:
      {
	/* [expr.context]: the left operand of a comma is a discarded-value
	   expression, and the lvalue-to-rvalue conversion is not applied to
	   it unless it is volatile-qualified.  A parameter that is that
	   operand is therefore not odr-used ([basic.def.odr]/5) -- this is
	   PR c++/126897, `void f (bool b) post ((b, true)) {}`.

	   convert_to_void has usually already replaced a side-effect-free
	   discarded operand with void_node, in which case there is nothing
	   here to find; but it does not fold a DEPENDENT predicate, which is
	   what a postcondition with a result-name-introducer has while it is
	   being parsed.  So recognise the shape rather than relying on it.

	   What is exempt is the set of potential results of the discarded
	   operand, not everything in it: in `(f (b), true)` the parameter is
	   an argument to the call, and that is an odr-use however the call's
	   value is discarded.  walk_discarded_operand computes that set.  */
	*walk_subtrees = 0;
	walk_discarded_operand (TREE_OPERAND (t, 0), d);
	cp_walk_tree_without_duplicates (&TREE_OPERAND (t, 1),
					 check_postcondition_odr_use_r, data);
	return NULL_TREE;
      }

    default:
      break;
    }

  if (PACK_EXPANSION_P (t))
    {
      *walk_subtrees = 0;
      return NULL_TREE;
    }

  if (TREE_CODE (t) != PARM_DECL)
    return NULL_TREE;

  /* [dcl.contract.func]/7 constrains a NON-REFERENCE parameter.  A use of
     a reference parameter is an INDIRECT_REF around the PARM_DECL, and the
     walk descends through it to the PARM_DECL, so the reference case has to
     be excluded explicitly.  */
  if (TYPE_REF_P (TREE_TYPE (t)))
    return NULL_TREE;

  /* The rule is about a parameter of f.  A lambda appearing in the predicate
     has parameters of its own, and they are none of f's business -- naming
     one is not a use of a parameter of f at all.

     Identify those positively rather than by "context is not FNDECL": a
     parameter's DECL_CONTEXT is not reliably the decl we were handed (a
     redeclaration chain has several, and the contract may hang off any of
     them), so an identity test would silently skip everything.  A lambda's
     operator(), on the other hand, is recognisable on its own terms -- and
     when the contracted function is a lambda, its own parameters still have
     DECL_CONTEXT == FNDECL and so are not skipped here.  */
  tree ctx = DECL_CONTEXT (t);
  if (ctx && ctx != d->fndecl
      && TREE_CODE (ctx) == FUNCTION_DECL
      && LAMBDA_FUNCTION_P (ctx))
    return NULL_TREE;

  /* The synthesised parameter holding the return value is artificial.  */
  if (DECL_ARTIFICIAL (t))
    return NULL_TREE;

  set_parm_used_in_post (t);

  if (!dependent_type_p (TREE_TYPE (t))
      && !CP_TYPE_CONST_P (TREE_TYPE (t)))
    {
      auto_diagnostic_group dg;
      location_t loc = DECL_SOURCE_LOCATION (t);
      if (d->loc != UNKNOWN_LOCATION)
	loc = d->loc;
      error_at (loc, "a value parameter used in a postcondition must be const");
      inform (DECL_SOURCE_LOCATION (t), "parameter declared here");
      d->diagnosed = true;
    }

  return NULL_TREE;
}

/* Apply [dcl.contract.func]/7 to the finished postcondition predicate
   CONDITION of FNDECL.

   This runs on the FINISHED expression rather than on each id-expression as
   it is parsed, because whether a parameter is odr-used is not known until
   the expression around it is.  The motivating case is PR c++/126897:

     void f (bool b) post ((b, true)) {}

   The left operand of a comma is a discarded-value expression to which the
   lvalue-to-rvalue conversion is not applied, so naming `b` there is not an
   odr-use ([basic.def.odr]/5) and the program is well-formed.  Checking at
   id-expression time could not know that -- the parser had an identifier, not
   an expression -- and rejected it.

   Nothing here has to model discarded-value contexts: convert_to_void already
   replaces a side-effect-free discarded operand with void_node, so such a
   parameter is simply not in the finished tree.  What the walk must do is
   avoid descending into the two places a parameter can appear without being
   odr-used and WITHOUT being dropped -- an unevaluated operand, and an
   unexpanded pack.

   If DIAGNOSED is non-null, *DIAGNOSED is set to whether an odr-use was
   diagnosed: the caller then marks the postcondition erroneous, so that
   nothing else -- its D4405 analysis evaluation in particular -- reports on
   it again (GCC-938).  */

void
check_postcondition_param_odr_uses (tree condition, tree fndecl,
				    location_t loc, bool *diagnosed)
{
  if (diagnosed)
    *diagnosed = false;
  if (!condition || condition == error_mark_node
      || TREE_CODE (condition) == DEFERRED_PARSE)
    return;

  postcondition_odr_use_data data = { fndecl, loc, false };
  cp_walk_tree_without_duplicates (&condition, check_postcondition_odr_use_r,
				   &data);
  if (diagnosed)
    *diagnosed = data.diagnosed;
}

/* [dcl.fct.def.coroutine]/5 says a coroutine behaves as if the top-level
   cv-qualifiers on all its parameters were removed -- so a by-value
   parameter is never truly const within the coroutine's real definition,
   however it is spelled.  A postcondition odr-using such a parameter must
   have it const ([dcl.contract.func]), and the two requirements cannot
   both be met.  The standard states the consequence outright, as a note
   ([dcl.fct.def.coroutine]/6): "An odr-use of a non-reference parameter in
   a postcondition assertion of a coroutine is ill-formed."

   A reference parameter is fine: its copy is bound to the same object.  So is
   a precondition, which is not subject to the const rule.

   This cannot be checked where the const rule is, because a function is not
   known to be a coroutine until its body has been parsed -- which is after
   its contracts.  FNDECL's parameters carry the flag set by
   check_postcondition_param_odr_uses, so the check is a walk once we know.  */

void
diagnose_coroutine_postcondition_params (tree fndecl)
{
  for (tree parm = DECL_ARGUMENTS (fndecl); parm; parm = DECL_CHAIN (parm))
    if (parm_used_in_post_p (parm))
      {
	auto_diagnostic_group d;
	error_at (DECL_SOURCE_LOCATION (parm),
		  "parameter %qD is odr-used in a postcondition of a "
		  "coroutine", parm);
	inform (DECL_SOURCE_LOCATION (parm),
		"a coroutine copies its parameters at the beginning of the "
		"replacement body, so a postcondition cannot name one; "
		"consider a reference parameter");
      }
}

/* OBJECT is the object expression of a class member access, or of a call,
   naming a static member or an enumerator, in a postcondition predicate
   being built.  Its potential results are not the member access's
   ([basic.def.odr]/3 includes them only for a non-static data member), so a
   parameter that is OBJECT is odr-used however the access is used -- but
   build_class_member_access_expr and keep_unused_object_arg drop an object
   without side effects, and the walk over the finished predicate never sees
   it (GCC-666).  Apply [dcl.contract.func]/7 to it here.  */

void
check_postcondition_object_odr_use (tree object)
{
  if (!flag_contracts
      || !processing_postcondition_predicate
      || cp_unevaluated_operand
      || !object)
    return;

  STRIP_ANY_LOCATION_WRAPPER (object);
  while (TREE_CODE (object) == VIEW_CONVERT_EXPR
	 || TREE_CODE (object) == NON_LVALUE_EXPR)
    {
      object = TREE_OPERAND (object, 0);
      STRIP_ANY_LOCATION_WRAPPER (object);
    }

  if (TREE_CODE (object) == PARM_DECL)
    check_postcondition_param_odr_uses (object, NULL_TREE, UNKNOWN_LOCATION);
}

/* [dcl.contract.func]/6: a deleted function, or one defaulted on its first
   declaration, shall not have a function-contract-specifier-seq.  DECL has
   just been marked as one of those; DELETED_P says which.  Diagnose a
   contract on it and drop the contract, rather than silently ignoring it --
   such a function has no body the precondition could guard, so the contract
   was simply doing nothing.

   The paragraph's third case, a *virtual* function, is deliberately not
   implemented here: P3097 is the proposal that lifts precisely that
   restriction, and this branch implements P3097.  A defaulted function that
   is not on its first declaration is unaffected -- the contract lives on the
   earlier declaration, which is where it belongs.  */

void
check_contract_on_defaulted_or_deleted (tree decl, bool deleted_p)
{
  if (!flag_contracts || !DECL_P (decl) || TREE_CODE (decl) != FUNCTION_DECL)
    return;

  /* Only a first-declaration default is restricted.  */
  if (!deleted_p && !DECL_DEFAULTED_IN_CLASS_P (decl))
    return;

  tree specs = get_fn_contract_specifiers (decl);
  if (!specs || specs == error_mark_node)
    return;

  error_at (DECL_SOURCE_LOCATION (decl),
	    deleted_p
	    ? G_("deleted function %qD cannot have a "
		 "function-contract-specifier")
	    : G_("function %qD defaulted on its first declaration cannot "
		 "have a function-contract-specifier"),
	    decl);
  remove_fn_contract_specifiers (decl);
}

/* Maps a FUNCTION_DECL to a TREE_LIST recording, for a declaration that
   duplicate_decls merged away, a parameter whose dependent type was not
   const: TREE_PURPOSE is its index in the parameter list and TREE_VALUE is
   the PARM_DECL itself, which carries both the type to substitute and the
   location to point at.

   Keyed on the function rather than on a parameter because which PARAMETERS
   survive a merge varies -- the definition's win -- while the surviving
   FUNCTION_DECL is always duplicate_decls' OLDDECL, which is also the pattern
   an instantiation is later made from.  Keying on a parameter loses any
   declaration that is neither the first nor the last of three.

   See check_postcondition_redecl_parm_types.  */

static GTY(()) hash_map<tree, tree, pch_traits> *postcondition_redecl_parms;

static GTY(()) hash_set<tree> *postcondition_redecl_parms_checked;

static void record_postcondition_redecl_parm (tree, unsigned, tree);

/* Carry the "odr used in a postcondition" property of the parameter T1 of
   OLDDECL over to the parameter T2 of a redeclaration or instantiation that
   corresponds to it, and check that T2 satisfies the const requirement.  */

static void
check_postcondition_parm_in_redecl (tree olddecl, tree t1, tree t2,
				    unsigned idx)
{
  /* Either of these declarations may be the one duplicate_decls discards, and
     which it is depends on things this function cannot see (a definition's
     parameters win).  Record both: recording is idempotent, and a parameter
     that does survive is checked directly anyway, so a redundant record costs
     nothing.  Recording only T1 loses the middle declaration of three
     (PR c++/127196).  Record them whether or not the pattern's predicate has
     marked T1: an access through a dependent object is only known to odr-use
     its object after substitution (GCC-663), and
     check_postcondition_redecl_parm_types looks only at parameters that turn
     out to be odr-used.  */
  record_postcondition_redecl_parm (olddecl, idx, t1);
  record_postcondition_redecl_parm (olddecl, idx, t2);

  if (!parm_used_in_post_p (t1))
    return;

  set_parm_used_in_post (t2);

  if (!dependent_type_p (TREE_TYPE (t2))
      && !CP_TYPE_CONST_P (TREE_TYPE (t2))
      && !TREE_READONLY (t2))
    {
      auto_diagnostic_group d;
      error_at (DECL_SOURCE_LOCATION (t2),
		"value parameter %qD used in a postcondition must be "
		"const", t2);
      inform (DECL_SOURCE_LOCATION (olddecl), "previous declaration here");
    }
}

/* PR c++/127196.  Record, against the surviving FUNCTION_DECL OLDDECL, that
   the parameter PARM at index IDX of a declaration being merged away has a
   dependent type that is not const.  [dcl.contract.func]/7 requires the
   corresponding parameter on all declarations to be const once the predicate
   odr-uses it, and a dependent type cannot be judged until the arguments are
   known -- by which time this declaration is gone.  */

static void
record_postcondition_redecl_parm (tree olddecl, unsigned idx, tree parm)
{
  /* uses_template_parms, not dependent_type_p: the caller also runs from
     tsubst_function_decl, where processing_template_decl is 0 and
     dependent_type_p asserts if handed a TEMPLATE_TYPE_PARM.  */
  if (!uses_template_parms (TREE_TYPE (parm))
      || CP_TYPE_CONST_P (TREE_TYPE (parm))
      || TREE_READONLY (parm))
    return;

  hash_map_maybe_create<hm_ggc> (postcondition_redecl_parms);

  tree &parms = postcondition_redecl_parms->get_or_insert (olddecl);
  for (tree p = parms; p; p = TREE_CHAIN (p))
    if (tree_to_uhwi (TREE_PURPOSE (p)) == idx && TREE_VALUE (p) == parm)
      return;
  parms = tree_cons (build_int_cstu (size_type_node, idx), parm, parms);
}

/* PR c++/127196.  PATTERN is a function template whose instantiation SPEC has
   just had its parameters substituted with ARGS.  Apply
   [dcl.contract.func]/7 to the declarations that were merged away: substitute
   each recorded parameter's type and require it to be const.

   Nothing is reported when the instantiation's own parameter is already
   non-const, because it has been diagnosed on its own account -- by the walk
   over the substituted predicate, or by check_postcondition_parm_in_redecl.
   This function exists precisely for the case where the surviving parameter
   looks fine and an earlier declaration did not.

   It runs twice per instantiation: from tsubst_function_decl, where the
   parameters carry what the pattern's predicate decided, and again from
   tsubst_contract once the predicate itself has been substituted -- a
   member access through a dependent object is only known to odr-use its
   object then (GCC-663).  It reports at most once for SPEC;
   POSTCONDITION_REDECL_PARMS_CHECKED holds the instantiations it has
   reported for.  (Not their parameters: regenerate_decl_from_template
   replaces those between the two calls.)  */

void
check_postcondition_redecl_parm_types (tree pattern, tree spec, tree args)
{
  if (!postcondition_redecl_parms)
    return;

  tree *slot = postcondition_redecl_parms->get (pattern);
  if (!slot)
    return;

  if (!postcondition_redecl_parms_checked)
    postcondition_redecl_parms_checked = hash_set<tree>::create_ggc (8);
  else if (postcondition_redecl_parms_checked->contains (spec))
    return;

  for (tree p = *slot; p; p = TREE_CHAIN (p))
    {
      unsigned idx = tree_to_uhwi (TREE_PURPOSE (p));
      tree recorded = TREE_VALUE (p);

      /* Find the instantiation's parameter at that index.  */
      tree sp = FUNCTION_FIRST_USER_PARM (spec);
      for (unsigned i = 0; sp && sp != void_list_node && i < idx; ++i)
	sp = TREE_CHAIN (sp);
      if (!sp || sp == void_list_node)
	continue;

      if (!parm_used_in_post_p (sp))
	continue;
      /* Already ill-formed on its own account; do not say it twice.  */
      if (!CP_TYPE_CONST_P (TREE_TYPE (sp)) && !TREE_READONLY (sp))
	continue;

      /* A parameter whose type became a reference by substitution is not
	 a value parameter, and is exempt.  */
      tree type = tsubst (TREE_TYPE (recorded), args, tf_none, NULL_TREE);
      if (type == error_mark_node || uses_template_parms (type)
	  || CP_TYPE_CONST_P (type) || TYPE_REF_P (type))
	continue;

      postcondition_redecl_parms_checked->add (spec);
      auto_diagnostic_group d;
      error_at (DECL_SOURCE_LOCATION (sp),
		"value parameter %qD used in a postcondition must be const",
		sp);
      inform (DECL_SOURCE_LOCATION (recorded),
	      "declared %qT here, which is not const", type);
      break;
    }
}

/* Check if parameters used in postconditions are const qualified on
   a redeclaration that does not specify contracts or on an instantiation
   of a function template.  */

void
check_postconditions_in_redecl (tree olddecl, tree newdecl)
{
  tree contract_spec = get_fn_contract_specifiers (olddecl);
  if (!contract_spec)
    return;

  tree first1 = FUNCTION_FIRST_USER_PARM (olddecl);
  tree first2 = FUNCTION_FIRST_USER_PARM (newdecl);

  /* A function parameter pack occupies a single slot in the pattern (OLDDECL)
     but expands to N parameters in the instantiation (NEWDECL), so the two
     lists cannot be walked in lockstep throughout: a pack that expands to
     nothing leaves NEWDECL's list the shorter of the two, and a parameter
     written after a pack sits at a different position in each list.

     What does correspond however the packs expand is the run of parameters
     before the first pack -- aligned from the front -- and the run after the
     last pack -- aligned from the back.  Walk those two runs, and skip the
     packs themselves: whether each odr-used element must be const is checked
     by the walk over the substituted predicate
     (check_postcondition_param_odr_uses), which by then sees the elements a
     pack expanded to.  Only a parameter written BETWEEN two packs is left
     unchecked here, which takes a second function parameter pack -- one that
     can never be deduced, and so never expands to anything.  */

  int len1 = 0, len2 = 0, first_pack = -1, last_pack = -1;
  for (tree t = first1; t && t != void_list_node; t = TREE_CHAIN (t), ++len1)
    if (DECL_PACK_P (t))
      {
	if (first_pack < 0)
	  first_pack = len1;
	last_pack = len1;
      }
  for (tree t = first2; t && t != void_list_node; t = TREE_CHAIN (t))
    ++len2;

  /* The run before the first pack, which is the whole list when there is no
     pack at all.  */
  tree t1 = first1, t2 = first2;
  unsigned idx = 0;
  for (int i = first_pack < 0 ? len1 : first_pack; i > 0;
       --i, t1 = TREE_CHAIN (t1), t2 = TREE_CHAIN (t2), ++idx)
    check_postcondition_parm_in_redecl (olddecl, t1, t2, idx);

  if (first_pack < 0)
    return;

  /* The run after the last pack, aligned from the back of each list.  The
     second skip is nonnegative: NEWDECL can fall short of OLDDECL by at most
     one parameter per pack, and the LAST_PACK + 1 parameters up to and
     including the last pack are at least that many.  */
  int skip1 = last_pack + 1;
  int skip2 = skip1 + (len2 - len1);
  gcc_checking_assert (skip2 >= 0);

  t1 = chain_index (skip1, first1);
  t2 = chain_index (skip2, first2);
  /* Index by position in NEWDECL's list, which is the one an instantiation's
     parameters correspond to.  */
  idx = skip2;
  for (; t1 && t1 != void_list_node && t2 && t2 != void_list_node;
       t1 = TREE_CHAIN (t1), t2 = TREE_CHAIN (t2), ++idx)
    check_postcondition_parm_in_redecl (olddecl, t1, t2, idx);
}

/* Map from FUNCTION_DECL to a FUNCTION_DECL for either the PRE_FN or POST_FN.
   These are used to parse contract conditions and are called inside the body
   of the guarded function.  */
static GTY(()) hash_map<tree, tree, pch_traits> *decl_pre_fn;
static GTY(()) hash_map<tree, tree, pch_traits> *decl_post_fn;

/* Given a pre or post function decl (for an outlined check function) return
   the decl for the function for which the outlined checks are being
   performed.  */
static GTY(()) hash_map<tree, tree, pch_traits> *orig_from_outlined;

/* Makes PRE the precondition function for FNDECL.  */

static void
set_precondition_function (tree fndecl, tree pre)
{
  gcc_assert (pre);
  hash_map_maybe_create<hm_ggc> (decl_pre_fn);
  gcc_checking_assert (!decl_pre_fn->get (fndecl));
  decl_pre_fn->put (fndecl, pre);

  hash_map_maybe_create<hm_ggc> (orig_from_outlined);
  gcc_checking_assert (!orig_from_outlined->get (pre));
  orig_from_outlined->put (pre, fndecl);
}

/* Makes POST the postcondition function for FNDECL.  */

static void
set_postcondition_function (tree fndecl, tree post)
{
  gcc_checking_assert (post);
  hash_map_maybe_create<hm_ggc> (decl_post_fn);
  gcc_checking_assert (!decl_post_fn->get (fndecl));
  decl_post_fn->put (fndecl, post);

  hash_map_maybe_create<hm_ggc> (orig_from_outlined);
  gcc_checking_assert (!orig_from_outlined->get (post));
  orig_from_outlined->put (post, fndecl);
}

/* For a given pre or post condition function, find the checked function.  */
tree
get_orig_for_outlined (tree fndecl)
{
  gcc_checking_assert (fndecl);
  tree *result = hash_map_safe_get (orig_from_outlined, fndecl);
  return result ? *result : NULL_TREE ;
}

/* For a given function OLD_FN set suitable names for NEW_FN (which is an
   outlined contract check) usually by appending '.pre' or '.post'.

   For functions with special meaning names (i.e. main and cdtors) we need to
   make special provisions and therefore handle all the contracts function
   name changes here, rather than requiring a separate update to mangle.cc.

   PRE specifies if we need an identifier for a pre or post contract check.  */

static void
contracts_fixup_names (tree new_fn, tree old_fn, bool pre, bool wrapper)
{
  bool cdtor = DECL_CXX_CONSTRUCTOR_P (old_fn)
	       || DECL_CXX_DESTRUCTOR_P (old_fn);
  const char *fname = IDENTIFIER_POINTER (DECL_NAME (old_fn));
  const char *append = wrapper ? "contract_wrapper"
			       : (pre ? "pre" : "post");
  size_t len = strlen (fname);
  /* Cdtor names have a space at the end.  We need to remove that space
     when forming the new identifier.  */
  char *nn = xasprintf ("%.*s%s%s",
			cdtor ? (int)len-1 : int(len),
			fname,
			JOIN_STR,
			append);
  DECL_NAME (new_fn) = get_identifier (nn);
  free (nn);

  /* Now do the mangled version.  */
  fname = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (old_fn));
  nn = xasprintf ("%s%s%s", fname, JOIN_STR, append);
  SET_DECL_ASSEMBLER_NAME (new_fn, get_identifier (nn));
  free (nn);
}

/* The P3098 capture-struct helpers, defined with the rest of the capture
   machinery below.  */
static tree get_postcondition_capture_struct_type (tree);
static bool has_postcondition_captures_p (tree);
static void append_capture_struct_args (tree, releasing_vec &);

/* True when CONTRACT is a postcondition of FNDECL that will actually be
   checked and that carries P3098 captures -- so it needs a capture struct.

   Ten places ask this, and five of them must agree on more than the answer:
   the capture-struct parameters added to __post_fn
   (build_contract_condition_function), the struct locals declared at the call
   site (declare_outlined_capture_struct_locals), the arguments pushed in
   (append_capture_struct_args) and the destructors emitted after
   (emit_outlined_capture_struct_dtors) are matched to each other BY POSITION,
   and get_capture_struct_params counts them to size that parameter list.
   They all walk the contracts in declaration order, so sharing this predicate
   is what keeps those sequences the same sequence; when it was written out ten
   times, one copy drifting would have silently misaligned a parameter list
   against its arguments, or sized it wrongly.

   ENSURE_EVALUATION_SEMANTIC is called with caller_side=false: this is about
   what the guarded function itself emits.  */

static const char *contract_dynamic_name (const_tree);
static tree find_std_contracts_type (tree);

static bool
active_postcondition_with_captures_p (tree contract, tree fndecl)
{
  if (TREE_CODE (contract) != POSTCONDITION_STMT
      || POSTCONDITION_CAPTURES (contract) == NULL_TREE)
    return false;
  contract_evaluation_semantic sem
    = ensure_evaluation_semantic (contract, fndecl, false);
  /* A dynamically selected postcondition may be evaluated whatever its
     compile-time default: the selection at the capture point decides
     (dynamic_selection_sites).  */
  return (contract_dynamic_name (contract)
	  || !contract_semantic_has_no_runtime_check (sem));
}

/* Build a declaration for the pre- or postcondition of a guarded FNDECL.  */

static tree
build_contract_condition_function (tree fndecl, bool pre)
{
  if (error_operand_p (fndecl))
    return error_mark_node;

  /* Start the copy.  */
  tree fn = copy_decl (fndecl);

  /* Don't propagate declaration attributes to the checking function,
     including the original contracts.  */
  DECL_ATTRIBUTES (fn) = NULL_TREE;

  /* If requested, disable optimisation of checking functions; this can, in
     some cases, prevent UB from eliding the checks themselves.  */
  if (flag_contract_disable_optimized_checks)
    DECL_ATTRIBUTES (fn)
      = tree_cons (get_identifier ("optimize"),
		   build_tree_list (NULL_TREE, build_string (3, "-O0")),
		   NULL_TREE);

  /* Now parse and add any internal representation of these attrs to the
     decl.  */
  if (DECL_ATTRIBUTES (fn))
    cplus_decl_attributes (&fn, DECL_ATTRIBUTES (fn), 0);

  /* A possible later optimization may delete unused args to prevent extra arg
     passing.  */

  /* Copy the function parameters, if present.  Disable warnings for them.

     A by-value parameter is passed BY REFERENCE to the checking function.
     The predicate runs in another function here, so a by-value parameter
     would be evaluated against a copy: a mutation the predicate performs
     would be invisible to the guarded function's body and to its caller,
     while a mutation of shared state in the same predicate would still be
     visible.  That partial application is not something the permission to
     elide a predicate in [basic.contract.eval] allows -- eliding produces
     all of a predicate's side effects or none of them, never some.  Having
     chosen to evaluate the predicate, we owe it the same objects an inline
     check would see.

     Artificial parameters (`this', __in_chrg, __vtt_parm) are left alone:
     they are not named by predicates and `this' is already a pointer.  So
     are types with TREE_ADDRESSABLE set -- a class with a non-trivial copy
     constructor or destructor is ALREADY passed by invisible reference, so
     the checking function and the guarded function share the object without
     any help.  Adding a reference on top of that produces a reference to the
     reference slot, and the predicate then writes through a pointer to the
     wrong thing.  (Measured: doing so turns a working case into a silently
     wrong one.)  */
  const bool by_ref = flag_contract_checks_outlined;
  int artificial = num_artificial_parms_for (fndecl);

  DECL_ARGUMENTS (fn) = NULL_TREE;
  if (DECL_ARGUMENTS (fndecl))
    {
      tree *last_a = &DECL_ARGUMENTS (fn);
      int idx = 0;
      for (tree p = DECL_ARGUMENTS (fndecl); p; p = TREE_CHAIN (p), ++idx)
	{
	  *last_a = copy_decl (p);
	  if (by_ref
	      && idx >= artificial
	      && !TYPE_REF_P (TREE_TYPE (*last_a))
	      && !TREE_ADDRESSABLE (TREE_TYPE (*last_a))
	      && TREE_TYPE (*last_a) != error_mark_node)
	    TREE_TYPE (*last_a)
	      = cp_build_reference_type (TREE_TYPE (*last_a), /*rval=*/false);
	  suppress_warning (*last_a);
	  DECL_CONTEXT (*last_a) = fn;
	  last_a = &TREE_CHAIN (*last_a);
	}
    }

  /* Build the argument type list from the parameters just created, so the
     two cannot drift apart.  For an iobj member function the `this' pointer
     is re-added by build_method_type_directly below, so drop it here.  */
  tree arg_types = NULL_TREE;
  tree *last = &arg_types;
  {
    tree p = DECL_ARGUMENTS (fn);
    if (p && DECL_IOBJ_MEMBER_FUNCTION_P (fndecl))
      p = TREE_CHAIN (p);
    for (; p; p = TREE_CHAIN (p))
      {
	*last = build_tree_list (NULL_TREE, TREE_TYPE (p));
	last = &TREE_CHAIN (*last);
      }
  }

  tree orig_fn_value_type = TREE_TYPE (TREE_TYPE (fn));
  if (!pre && !VOID_TYPE_P (orig_fn_value_type))
    {
      /* For post contracts that deal with a non-void function, append a
	 parameter to pass the return value.  By reference, for the same
	 reason the by-value parameters above are: a postcondition that
	 mutates the result binding must be mutating the returned object,
	 not a copy of it, exactly as it does for an inline check.  */
      tree r_type = orig_fn_value_type;
      if (by_ref && !TYPE_REF_P (r_type) && !TREE_ADDRESSABLE (r_type)
	  && r_type != error_mark_node)
	r_type = cp_build_reference_type (r_type, /*rval=*/false);
      tree name = get_identifier ("__r");
      tree parm = build_lang_decl (PARM_DECL, name, r_type);
      DECL_CONTEXT (parm) = fn;
      DECL_ARTIFICIAL (parm) = true;
      suppress_warning (parm);
      DECL_ARGUMENTS (fn) = chainon (DECL_ARGUMENTS (fn), parm);
      *last = build_tree_list (NULL_TREE, r_type);
      last = &TREE_CHAIN (*last);
    }

  /* P3098: For each active postcondition with captures, append a reference
     parameter for the capture-state struct.  Both __pre_fn and __post_fn
     get the same struct reference parameters.  */
  if (tree contracts = get_fn_contract_specifiers (fndecl))
    {
      unsigned cap_idx = 0;
      for (tree contract : tree_vec_range (contracts))
	{
	  if (!active_postcondition_with_captures_p (contract, fndecl))
	    continue;

	  tree struct_type = get_postcondition_capture_struct_type (contract);
	  tree ref_type
	    = cp_build_reference_type (struct_type, /*rval=*/false);

	  char buf[32];
	  snprintf (buf, sizeof buf, "__captures_%u", cap_idx++);
	  tree parm = build_lang_decl (PARM_DECL, get_identifier (buf),
				       ref_type);
	  DECL_CONTEXT (parm) = fn;
	  DECL_ARTIFICIAL (parm) = true;
	  suppress_warning (parm);
	  DECL_ARGUMENTS (fn) = chainon (DECL_ARGUMENTS (fn), parm);
	  *last = build_tree_list (NULL_TREE, ref_type);
	  last = &TREE_CHAIN (*last);
	}
    }

  *last = void_list_node;

  tree adjusted_type = NULL_TREE;

  /* The handlers are void fns.  */
  if (DECL_IOBJ_MEMBER_FUNCTION_P (fndecl))
    adjusted_type = build_method_type_directly (DECL_CONTEXT (fndecl),
						void_type_node,
						arg_types);
  else
    adjusted_type = build_function_type (void_type_node, arg_types);

  /* If the original function is noexcept, build a noexcept function.
     Also build a noexcept function (P4298) when every contract this
     outlined function actually checks -- preconditions, and postconditions
     with captures to initialize, for the .pre function; postconditions for
     the .post function -- has a statically nonthrowing semantic, even if the
     original function itself may throw.  */
  if (flag_exceptions
      && (type_noexcept_p (TREE_TYPE (fndecl))
	  || (flag_contracts_p4298
	      && all_contracts_statically_nonthrowing
		   (get_fn_contract_specifiers (fndecl), fndecl,
		    pre ? PRECONDITION_STMT : POSTCONDITION_STMT))))
    adjusted_type = build_exception_variant (adjusted_type, noexcept_true_spec);

  TREE_TYPE (fn) = adjusted_type;
  DECL_RESULT (fn) = NULL_TREE; /* Let the start function code fill it in.  */

  /* The contract check functions are never a cdtor, nor virtual.  */
  DECL_CXX_DESTRUCTOR_P (fn) = DECL_CXX_CONSTRUCTOR_P (fn) = 0;
  DECL_VIRTUAL_P (fn) = false;

  /* Append .pre / .post to a usable name for the original function.  */
  contracts_fixup_names (fn, fndecl, pre, /*wrapper*/false);

  DECL_INITIAL (fn) = NULL_TREE;
  CONTRACT_HELPER (fn) = pre ? ldf_contract_pre : ldf_contract_post;
  /* We might have a pre/post for a wrapper.  */
  DECL_CONTRACT_WRAPPER (fn) = DECL_CONTRACT_WRAPPER (fndecl);

  /* Make these functions internal if we can, i.e. if the guarded function is
     not vague linkage, or if we can put them in a comdat group with the
     guarded function.  */
  if (!DECL_WEAK (fndecl) || HAVE_COMDAT_GROUP)
    {
      TREE_PUBLIC (fn) = false;
      DECL_EXTERNAL (fn) = false;
      DECL_WEAK (fn) = false;
      DECL_COMDAT (fn) = false;

      /* We may not have set the comdat group on the guarded function yet.
	 If we haven't, we'll add this to the same group in comdat_linkage
	 later.  Otherwise, add it to the same comdat group now.  */
      if (DECL_ONE_ONLY (fndecl))
	{
	  symtab_node *n = symtab_node::get (fndecl);
	  cgraph_node::get_create (fn)->add_to_same_comdat_group (n);
	}

    }

  DECL_INTERFACE_KNOWN (fn) = true;
  DECL_ARTIFICIAL (fn) = true;
  suppress_warning (fn);

  return fn;
}

/* Build the precondition checking function for FNDECL.  Also needed when
   postconditions have captures, since __pre_fn handles capture init.  */

static tree
build_precondition_function (tree fndecl)
{
  if (!has_active_preconditions (fndecl)
      && !has_postcondition_captures_p (fndecl))
    return NULL_TREE;

  return build_contract_condition_function (fndecl, /*pre=*/true);
}

/* Build the postcondition checking function for FNDECL.  If the return
   type is undeduced, don't build the function yet.  We do that in
   apply_deduced_return_type.  */

static tree
build_postcondition_function (tree fndecl)
{
  if (!has_active_postconditions (fndecl))
    return NULL_TREE;

  tree type = TREE_TYPE (TREE_TYPE (fndecl));
  if (is_auto (type))
    return NULL_TREE;

  return build_contract_condition_function (fndecl, /*pre=*/false);
}

/* If we're outlining the contract, build the functions to do the
   precondition and postcondition checks, and associate them with
   the function decl FNDECL.
 */

static void
build_contract_function_decls (tree fndecl)
{
  /* Build the pre/post functions (or not).  */
  if (!get_precondition_function (fndecl))
    if (tree pre = build_precondition_function (fndecl))
      set_precondition_function (fndecl, pre);

  if (!get_postcondition_function (fndecl))
    if (tree post = build_postcondition_function (fndecl))
      set_postcondition_function (fndecl, post);
}

/* Map from a callee FUNCTION_DECL to a TREE_LIST of (tuple, wrapdecl) pairs.
   A single callee may have several caller-side wrappers, one per distinct
   resolved caller-semantic tuple (P3595).  Each list node uses:
     TREE_PURPOSE = the caller-semantic tuple (see below), and
     TREE_VALUE   = the wrapper FUNCTION_DECL.
   The tuple is itself a TREE_LIST whose Nth TREE_VALUE is an INTEGER_CST
   giving the resolved caller-side semantic for the Nth contract in the
   callee's full contract-specifier list; NULL_TREE is the sentinel empty
   tuple used for virtual (P3097) wrappers, which share a single wrapper.  */

static GTY(()) hash_map<tree, tree, pch_traits> *decl_wrapper_fn = nullptr;

/* Map from the function decl of a wrapper to the function that it wraps.  */

static GTY(()) hash_map<tree, tree, pch_traits> *decl_for_wrapper = nullptr;

/* Map from a wrapper FUNCTION_DECL to its caller-semantic tuple (a TREE_LIST
   of INTEGER_CSTs, or NULL_TREE for the virtual sentinel).  Read by the
   definition pass to set each copied contract's evaluation semantic.  */

static GTY(()) hash_map<tree, tree, pch_traits> *decl_wrapper_tuple = nullptr;

/* Return true if two caller-semantic tuples are element-wise equal.  Each
   entry's TREE_VALUE is the resolved semantic (INTEGER_CST); its TREE_PURPOSE
   is the P3595 dynamic descriptor (NULL_TREE when the entry is not dynamic,
   else a TREE_LIST whose TREE_PURPOSE is the selector name IDENTIFIER and
   whose TREE_VALUE is the INTEGER_CST CONTRACT_DYNAMIC packs).  Two call
   sites that resolve to different descriptors must key distinct wrappers, so
   the descriptor is part of the comparison.  */

static bool
wrapper_tuples_equal (tree a, tree b)
{
  for (; a && b; a = TREE_CHAIN (a), b = TREE_CHAIN (b))
    {
      if (tree_to_uhwi (TREE_VALUE (a)) != tree_to_uhwi (TREE_VALUE (b)))
	return false;
      tree da = TREE_PURPOSE (a), db = TREE_PURPOSE (b);
      if ((da == NULL_TREE) != (db == NULL_TREE))
	return false;
      /* IDENTIFIER_NODEs are interned, so the name compares by pointer.  */
      if (da && db
	  && (TREE_PURPOSE (da) != TREE_PURPOSE (db)
	      || (tree_to_uhwi (TREE_VALUE (da))
		  != tree_to_uhwi (TREE_VALUE (db)))))
	return false;
    }
  return a == NULL_TREE && b == NULL_TREE;
}

/* Store TUPLE as the caller-semantic tuple for wrapper WRAPDECL.  */

static void
set_wrapper_tuple (tree wrapdecl, tree tuple)
{
  hash_map_maybe_create<hm_ggc> (decl_wrapper_tuple);
  decl_wrapper_tuple->put (wrapdecl, tuple);
}

/* Return the caller-semantic tuple stored for wrapper WRAPDECL.  */

static tree
get_wrapper_tuple (tree wrapdecl)
{
  tree *result = hash_map_safe_get (decl_wrapper_tuple, wrapdecl);
  return result ? *result : NULL_TREE;
}

/* True if WRAPDECL was created for a virtual dispatch (P3097): its stored
   tuple is the NULL_TREE sentinel.  A qualified call to a virtual function
   gets a caller-tuple wrapper instead, which calls the function directly and
   evaluates its contracts caller-side.  */

static bool
wrapper_for_virtual_dispatch_p (tree wrapdecl)
{
  tree *result = hash_map_safe_get (decl_wrapper_tuple, wrapdecl);
  return result && *result == NULL_TREE;
}

/* Return the resolved caller-side semantic for the contract at (full-list)
   position POSITION in WRAPDECL's stored tuple, or CES_IGNORE if absent.  */

static unsigned char
get_wrapper_tuple_at (tree wrapdecl, unsigned position)
{
  tree tuple = get_wrapper_tuple (wrapdecl);
  /* If a tuple is stored, POSITION must be one of its elements: the tuple
     has one entry per contract in the callee's full contract-specifier
     list, and callers only ever query positions from that same list (see
     compute_caller_semantic_tuple, copy_and_remap_contracts, and
     define_one_contract_wrapper_func).  Falling off the end here would
     otherwise silently return CES_IGNORE and mask a future alignment bug.
     A NULL_TREE tuple (no tuple stored for WRAPDECL) is the legitimate
     sentinel case and is not subject to this check.  */
  gcc_checking_assert (!tuple || position < (unsigned) list_length (tuple));
  for (unsigned i = 0; tuple; tuple = TREE_CHAIN (tuple), i++)
    if (i == position)
      return (unsigned char) tree_to_uhwi (TREE_VALUE (tuple));
  return (unsigned char) CES_IGNORE;
}

/* Return the P3595 dynamic-selector descriptor (TREE_PURPOSE) for the contract
   at (full-list) POSITION in WRAPDECL's stored tuple, or NULL_TREE if that
   entry is not dynamic.  The descriptor is a TREE_LIST whose TREE_PURPOSE is
   the selector name IDENTIFIER and whose TREE_VALUE is the packed
   INTEGER_CST -- the same layout the callee-side
   CONTRACT_DYNAMIC cache uses.  */

static tree
get_wrapper_dyn_at (tree wrapdecl, unsigned position)
{
  tree tuple = get_wrapper_tuple (wrapdecl);
  gcc_checking_assert (!tuple || position < (unsigned) list_length (tuple));
  for (unsigned i = 0; tuple; tuple = TREE_CHAIN (tuple), i++)
    if (i == position)
      return TREE_PURPOSE (tuple);
  return NULL_TREE;
}

/* Find an existing wrapper of FNDECL whose stored tuple equals TUPLE, or
   NULL_TREE if none.  */

static tree
find_wrapper_for_tuple (tree fndecl, tree tuple)
{
  tree *listp = hash_map_safe_get (decl_wrapper_fn, fndecl);
  if (!listp)
    return NULL_TREE;
  for (tree p = *listp; p; p = TREE_CHAIN (p))
    if (wrapper_tuples_equal (TREE_PURPOSE (p), tuple))
      return TREE_VALUE (p);
  return NULL_TREE;
}

/* Record WRAPPER as the wrapper of FNDECL for caller-semantic tuple TUPLE.  */

static void
set_wrapper_for_tuple (tree fndecl, tree tuple, tree wrapper)
{
  gcc_checking_assert (wrapper && fndecl);
  hash_map_maybe_create<hm_ggc> (decl_wrapper_fn);
  tree *listp = decl_wrapper_fn->get (fndecl);
  tree node = tree_cons (tuple, wrapper, listp ? *listp : NULL_TREE);
  decl_wrapper_fn->put (fndecl, node);

  /* We need to know the wrapped function when composing the diagnostic.  */
  hash_map_maybe_create<hm_ggc> (decl_for_wrapper);
  gcc_checking_assert (decl_for_wrapper && !decl_for_wrapper->get (wrapper));
  decl_for_wrapper->put (wrapper, fndecl);
}

/* Given a wrapper function WRAPPER, find the original function decl.  */

tree
get_orig_func_for_wrapper (tree wrapper)
{
  gcc_checking_assert (wrapper);
  tree *result = hash_map_safe_get (decl_for_wrapper, wrapper);
  return result ? *result : NULL_TREE;
}

/* Build a declaration for the contract wrapper of a caller FNDECL.
   We're making a caller side contract check wrapper. For caller side contract
   checks, postconditions are only checked if check_post is true.
   Defer the attachment of the contracts to this function until the callee
   is non-dependent, or we get cases where the conditions can be non-dependent
   but still need tsubst-ing.  */

static tree
build_contract_wrapper_function (tree fndecl)
{
  if (error_operand_p (fndecl))
    return error_mark_node;

  /* We should not be trying to build wrappers for templates or functions that
     are still dependent.  */
  gcc_checking_assert (!processing_template_decl
		       && !TYPE_DEPENDENT_P (TREE_TYPE (fndecl)));

  location_t loc = DECL_SOURCE_LOCATION (fndecl);

  /* Fill in the names later.  */
  tree wrapdecl
    = build_lang_decl_loc (loc, FUNCTION_DECL, NULL_TREE, TREE_TYPE (fndecl));

  /* Put the wrapper in the same context as the callee.  */
  DECL_CONTEXT (wrapdecl) = DECL_CONTEXT (fndecl);

  /* This declaration is a contract wrapper function.  */
  DECL_CONTRACT_WRAPPER (wrapdecl) = true;

  contracts_fixup_names (wrapdecl, fndecl, /*pre*/false, /*wrapper*/true);

  /* A single callee can have several, non-identical wrappers coexisting in
     one TU (P3595 caller-side: distinct call sites resolving to distinct
     caller-semantic tuples, e.g. different dynamic selectors -- see
     wrapper_tuples_equal).  All wrappers are internal (TREE_PUBLIC is
     cleared below), but they are still separate definitions and need
     distinct names or their identical ".contract_wrapper"-suffixed
     assembler names collide.  The first wrapper for FNDECL keeps the plain
     name for readability; subsequent ones get a numeric discriminator.  */
  if (tree *listp = hash_map_safe_get (decl_wrapper_fn, fndecl))
    {
      unsigned idx = (unsigned) list_length (*listp);
      char *nn = xasprintf ("%s.%u",
			    IDENTIFIER_POINTER (DECL_NAME (wrapdecl)), idx);
      DECL_NAME (wrapdecl) = get_identifier (nn);
      free (nn);
      nn = xasprintf ("%s.%u",
		      IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (wrapdecl)), idx);
      SET_DECL_ASSEMBLER_NAME (wrapdecl, get_identifier (nn));
      free (nn);
    }

  DECL_SOURCE_LOCATION (wrapdecl) = loc;
  /* The declaration was implicitly generated by the compiler.  */
  DECL_ARTIFICIAL (wrapdecl) = true;
  /* Declaration, no definition yet.  */
  DECL_INITIAL (wrapdecl) = NULL_TREE;

  /* Let the start function code fill in the result decl.  */
  DECL_RESULT (wrapdecl) = NULL_TREE;

  /* Copy the function parameters, if present.  Suppress (e.g. unused)
     warnings on them.  */
  DECL_ARGUMENTS (wrapdecl) = NULL_TREE;
  if (tree p = DECL_ARGUMENTS (fndecl))
    {
      tree *last_a = &DECL_ARGUMENTS (wrapdecl);
      for (; p; p = TREE_CHAIN (p))
	{
	  *last_a = copy_decl (p);
	  suppress_warning (*last_a);
	  DECL_CONTEXT (*last_a) = wrapdecl;
	  last_a = &TREE_CHAIN (*last_a);
	}
    }

  /* Copy selected attributes from the original function.  A call through
     the wrapper returns exactly when the callee does, so a noreturn callee
     makes a noreturn wrapper (TREE_THIS_VOLATILE), which flow analysis such
     as -Wreturn-type relies on.  */
  TREE_USED (wrapdecl) = TREE_USED (fndecl);
  TREE_THIS_VOLATILE (wrapdecl) = TREE_THIS_VOLATILE (fndecl);

  /* A constexpr/consteval callee needs an equally-constexpr wrapper: when a
     wrapper is interposed on a call (e.g. a virtual function's contract check),
     a non-constexpr wrapper would make the whole call unusable in constant
     evaluation.  The pre/post condition functions inherit this via copy_decl;
     the wrapper is built from scratch, so propagate it explicitly.  */
  DECL_DECLARED_CONSTEXPR_P (wrapdecl) = DECL_DECLARED_CONSTEXPR_P (fndecl);
  if (DECL_IMMEDIATE_FUNCTION_P (fndecl))
    SET_DECL_IMMEDIATE_FUNCTION_P (wrapdecl);

  /* Copy any alignment added.  */
  if (DECL_ALIGN (fndecl))
    SET_DECL_ALIGN (wrapdecl, DECL_ALIGN (fndecl));
  DECL_USER_ALIGN (wrapdecl) = DECL_USER_ALIGN (fndecl);

  /* Make this function internal.  */
  TREE_PUBLIC (wrapdecl) = false;
  DECL_EXTERNAL (wrapdecl) = false;
  DECL_WEAK (wrapdecl) = false;

  /* We know this is an internal function.  */
  DECL_INTERFACE_KNOWN (wrapdecl) = true;

  /* The wrapper of a C-variadic callee passes the variadic arguments on
     with __builtin_va_arg_pack, which needs it inlined at every call; it is
     only ever called directly, so it always can be (DECISIONS.md K12).  */
  if (stdarg_p (TREE_TYPE (fndecl)))
    {
      DECL_DECLARED_INLINE_P (wrapdecl) = true;
      DECL_DISREGARD_INLINE_LIMITS (wrapdecl) = true;
      DECL_ATTRIBUTES (wrapdecl)
	= tree_cons (get_identifier ("always_inline"), NULL_TREE,
		     DECL_ATTRIBUTES (wrapdecl));
    }
  return wrapdecl;
}

/* Return the wrapper of FNDECL whose caller-semantic tuple is TUPLE,
   creating it (and recording TUPLE) if it does not yet exist.  */

static tree
get_or_create_contract_wrapper_function (tree fndecl, tree tuple)
{
  tree wrapdecl = find_wrapper_for_tuple (fndecl, tuple);
  if (!wrapdecl)
    {
      wrapdecl = build_contract_wrapper_function (fndecl);
      set_wrapper_for_tuple (fndecl, tuple, wrapdecl);
      set_wrapper_tuple (wrapdecl, tuple);
    }
  return wrapdecl;
}

void
start_function_contracts (tree fndecl)
{
  if (error_operand_p (fndecl))
    return;

  /* Parse any predicate that is still deferred, before anything below reads
     it.  A function contract is token-cached at its declarator -- the
     grammar puts the seq after the complete declarator, where the function's
     parameters are no longer in scope -- and this is the first point at
     which they are back: start_function has just run store_parm_decls.
     Everything below needs the parsed form, the shadow check to have a
     result name and the outlined contract functions to have the capture
     VAR_DECLs rather than the identifiers the deferred form carries.

     Ahead of handle_contracts_p deliberately: that is false while
     processing_template_decl, but a template's pattern still has to be
     PARSED -- leaving it deferred means tsubst_contract meets a
     DEFERRED_PARSE when the template is instantiated.  Reading the source
     text is not conditional on whether checks will be emitted.  */
  cp_late_parse_function_contracts (fndecl);

  /* A lambda's contracts can name its explicit captures, whose proxies
     start_lambda_function pushes only after this, so they are still
     deferred here; cp_parser_lambda_body calls again once they are in
     scope.  */
  if (!handle_contracts_p (fndecl)
      || (LAMBDA_FUNCTION_P (fndecl)
	  && contract_any_deferred_p (get_fn_contract_specifiers (fndecl))))
    return;

  /* A result name that failed to parse leaves the postcondition unusable.
     (One that conflicts with a parameter is diagnosed where it is made, by
     check_result_name_conflict.)  */
  if (tree specs = get_fn_contract_specifiers (fndecl))
    for (tree ca : tree_vec_range (specs))
      if (POSTCONDITION_P (ca)
	  && POSTCONDITION_IDENTIFIER (ca) == error_mark_node)
	CONTRACT_CONDITION (ca) = error_mark_node;

  if (!contract_any_active_p (fndecl))
    return;

  /* A postcondition check runs after the returned object has been
     initialized, and a violation handler that throws unwinds straight
     through it.  Record that as what it is -- a cleanup that might throw --
     so maybe_set_retval_sentinel builds the sentinel even for a function
     whose body has no throwing cleanup of its own, and so the one cleanup
     that results is spliced around the contracts block rather than the body.
     Without it the returned object leaks (PR c++/127414), and a cleanup
     spliced around both the body and the contracts block destroys it twice
     (PR c++/127281).

     A coroutine needs no special case here even though its ramp cannot use
     the sentinel: the coroutine transform clears throwing_cleanup itself
     (see coroutines.cc) before contracts are applied, so no sentinel is ever
     created and maybe_apply_function_contracts falls back to wrapping the
     checks directly.  */
  if (postconditions_may_throw_p (fndecl))
    {
      cp_function_chain->throwing_cleanup = true;
      cp_function_chain->defer_retval_cleanup = true;
    }

  /* If we are expanding contract assertions inline then no need to declare
     the outline function decls.  */
  if (!flag_contract_checks_outlined)
    return;

  /* Contracts may have just been added without a chance to parse them, though
     we still need the PRE_FN available to generate a call to it.  */
  /* Do we already have declarations generated ? */
  if (!DECL_PRE_FN (fndecl) && !DECL_POST_FN (fndecl))
    build_contract_function_decls (fndecl);
}

void
maybe_update_postconditions (tree fndecl)
{
  /* Update any postconditions and the postcondition checking function
     as needed.  If there are postconditions, we'll use those to rewrite
     return statements to check postconditions.  */
  if (has_active_postconditions (fndecl))
    {
      rebuild_postconditions (fndecl);
      tree post = build_postcondition_function (fndecl);
      if (post)
	set_postcondition_function (fndecl, post);
    }
}

/* Build and return an argument list containing all the parameters of the
   (presumably guarded) function decl FNDECL.  This can be used to forward
   all of FNDECL arguments to a function taking the same list of arguments
   -- namely the unchecked form of FNDECL.

   We use CALL_FROM_THUNK_P instead of forward_parm for forwarding
   semantics.  */

static vec<tree, va_gc> *
build_arg_list (tree fndecl)
{
  vec<tree, va_gc> *args = make_tree_vector ();
  for (tree t = DECL_ARGUMENTS (fndecl); t; t = DECL_CHAIN (t))
    vec_safe_push (args, t);
  return args;
}

/* Adjust ARGS, built from the guarded function's own parameters, for a call
   to one of its outlined checking functions FN.  Those take by-value
   parameters by reference (see build_contract_condition_function), so an
   argument whose corresponding parameter is a reference the original was not
   must have its address taken.  Leaves everything else alone, so this is the
   identity when the checks are not outlined.  */

static void
adjust_args_for_by_ref_params (tree fn, vec<tree, va_gc> *args)
{
  tree parm = DECL_ARGUMENTS (fn);
  unsigned i;
  tree arg;
  FOR_EACH_VEC_SAFE_ELT (args, i, arg)
    {
      if (!parm)
	break;
      if (TYPE_REF_P (TREE_TYPE (parm))
	  && !TYPE_REF_P (TREE_TYPE (arg))
	  && TREE_TYPE (arg) != error_mark_node)
	(*args)[i] = build_address (arg);
      parm = DECL_CHAIN (parm);
    }
}

/* Build and return a thunk like call to FUNC from CALLER using the supplied
   arguments.  The call is like a thunk call in the fact that we do not
   want to create additional copies of the arguments.  We can not simply reuse
   the thunk machinery as it does more than we want.  More specifically, we
   don't want to mark the calling function as `DECL_THUNK_P` for this
   particular purpose, we only want the special treatment for the parameters
   of the call we are about to generate.  We temporarily mark the calling
   function as DECL_THUNK_P so build_call_a does the right thing.  */

static tree
build_thunk_like_call (tree func, int n, tree *argarray)
{
  bool old_decl_thunk_p = DECL_THUNK_P (current_function_decl);
  LANG_DECL_FN_CHECK (current_function_decl)->thunk_p  = true;

  tree call = build_call_a (func, n, argarray);

  /* Revert the `DECL_THUNK_P` flag.  */
  LANG_DECL_FN_CHECK (current_function_decl)->thunk_p = old_decl_thunk_p;

  /* Mark the call as a thunk call to allow for correct gimplification
   of the arguments.  */
  CALL_FROM_THUNK_P (call) = true;

  return call;
}

/* If we have a precondition function and it's valid, call it.  */

static void
add_pre_condition_fn_call (tree fndecl)
{
  gcc_checking_assert (DECL_PRE_FN (fndecl)
		       && DECL_PRE_FN (fndecl) != error_mark_node);

  releasing_vec args = build_arg_list (fndecl);
  adjust_args_for_by_ref_params (DECL_PRE_FN (fndecl), args);
  append_capture_struct_args (fndecl, args);
  tree call = build_thunk_like_call (DECL_PRE_FN (fndecl),
				     args->length (), args->address ());

  finish_expr_stmt (call);
}

/* Returns the parameter corresponding to the return value of a guarded
   function FNDECL.  Returns NULL_TREE if FNDECL has no postconditions or
   is void.  */

static tree
get_postcondition_result_parameter (tree fndecl)
{
  if (!fndecl || fndecl == error_mark_node)
    return NULL_TREE;

  if (VOID_TYPE_P (TREE_TYPE (TREE_TYPE (fndecl))))
    return NULL_TREE;

  tree post = DECL_POST_FN (fndecl);
  if (!post || post == error_mark_node)
    return NULL_TREE;

  /* The last param is the return value.  */
  return tree_last (DECL_ARGUMENTS (post));
}

/* Build and add a call to the post-condition checking function, when that
   is in use.  */

static void
add_post_condition_fn_call (tree fndecl)
{
  gcc_checking_assert (DECL_POST_FN (fndecl)
		       && DECL_POST_FN (fndecl) != error_mark_node);

  releasing_vec args = build_arg_list (fndecl);

  /* When the result is passed by reference (outlined checks), a mutation the
     postcondition performs has to reach the object the caller receives.

     How that is arranged depends on where the result lives, and the two
     cases line up with the two rows of [dcl.contract.res] Example 2:

     - Returned in memory: DECL_RESULT is addressable and is the object the
       caller sees, so hand over its address directly.  Example 2 requires
       `&r == ptr' to hold for such a type, and a temporary here would break
       that.

     - Returned in a register: DECL_RESULT is a gimple register, so taking
       its address spills it to a temporary of the gimplifier's choosing and
       the write lands in the spill and is lost.  Spill it ourselves instead,
       pass the address of that, and copy the result back afterwards -- the
       register is updated after the check runs.  Example 2 leaves the
       identity of `r' unspecified for this case, so the temporary is
       permitted; what is not permitted is evaluating the predicate and
       discarding what it did.  */
  tree retval_tmp = NULL_TREE;
  if (get_postcondition_result_parameter (fndecl))
    {
      tree result = DECL_RESULT (fndecl);
      tree restype = TREE_TYPE (TREE_TYPE (fndecl));

      if (flag_contract_checks_outlined
	  && result
	  && !aggregate_value_p (restype, fndecl))
	{
	  retval_tmp = build_decl (DECL_SOURCE_LOCATION (fndecl), VAR_DECL,
				   get_identifier ("__contract_retval"),
				   restype);
	  DECL_ARTIFICIAL (retval_tmp) = 1;
	  DECL_IGNORED_P (retval_tmp) = 1;
	  DECL_CONTEXT (retval_tmp) = fndecl;
	  TREE_ADDRESSABLE (retval_tmp) = 1;
	  layout_decl (retval_tmp, 0);
	  pushdecl (retval_tmp);
	  add_decl_expr (retval_tmp);
	  finish_expr_stmt (cp_build_init_expr (retval_tmp, result));
	  vec_safe_push (args, retval_tmp);
	}
      else
	vec_safe_push (args, result);
    }

  adjust_args_for_by_ref_params (DECL_POST_FN (fndecl), args);
  append_capture_struct_args (fndecl, args);
  tree call = build_thunk_like_call (DECL_POST_FN (fndecl),
				     args->length (), args->address ());
  finish_expr_stmt (call);

  /* Copy back whatever the postcondition did to the spilled result.  */
  if (retval_tmp)
    finish_expr_stmt (build2 (MODIFY_EXPR, TREE_TYPE (retval_tmp),
			      DECL_RESULT (fndecl), retval_tmp));
}

/* Copy (possibly a sub-set of) contracts from CONTRACTS on FNDECL.  */

static tree
copy_contracts_list (tree contracts, tree fndecl,
		     contract_match_kind remap_kind = cmk_all)
{
  if (!contracts)
    return NULL_TREE;

  auto_vec<tree> copies (TREE_VEC_LENGTH (contracts));
  for (tree contract : tree_vec_range (contracts))
    {
      if ((remap_kind == cmk_pre
	   && TREE_CODE (contract) == POSTCONDITION_STMT)
	  || (remap_kind == cmk_post
	      && TREE_CODE (contract) == PRECONDITION_STMT))
	continue;

      tree c = copy_node (contract);

      /* Parameters, the result binding and captures stay as they are, but a
	 variable the predicate declares itself -- in a statement expression
	 -- gets a copy of its own.  remap_decls rebuilds a BIND_EXPR's
	 variable list from the declarations it newly made, so a retained one
	 would silently drop out of it, and the gimplifier would then meet a
	 variable that no BIND_EXPR declares.  */
      struct predicate_copy_data : copy_body_data
      {
	hash_set<tree> *locals;
      } id;
      hash_map<tree, tree> decl_map;
      hash_set<tree> locals;

      memset (&id, 0, sizeof (id));

      id.src_fn = fndecl;
      id.dst_fn = fndecl;
      id.src_cfun = DECL_STRUCT_FUNCTION (fndecl);
      id.decl_map = &decl_map;

      auto collect_locals = [] (tree *tp, int *, void *data) -> tree
	{
	  if (TREE_CODE (*tp) == BIND_EXPR)
	    for (tree var = BIND_EXPR_VARS (*tp); var; var = DECL_CHAIN (var))
	      static_cast<hash_set<tree> *> (data)->add (var);
	  return NULL_TREE;
	};
      cp_walk_tree_without_duplicates (&CONTRACT_CONDITION (c),
				       collect_locals, &locals);
      id.locals = &locals;
      id.copy_decl = [] (tree decl, copy_body_data *cb) -> tree
	{
	  if (static_cast<predicate_copy_data *> (cb)->locals->contains (decl))
	    return copy_decl_no_change (decl, cb);
	  return decl;
	};

      id.transform_call_graph_edges = CB_CGE_DUPLICATE;
      id.transform_new_cfg = false;
      id.transform_return_to_modify = false;
      id.transform_parameter = true;

      /* Make sure not to unshare trees behind the front-end's back
	 since front-end specific mechanisms may rely on sharing.  */
      id.regimplify = false;
      id.do_not_unshare = true;
      id.do_not_fold = true;

      /* We're not inside any EH region.  */
      id.eh_lp_nr = 0;
      walk_tree (&CONTRACT_CONDITION (c), copy_tree_body_r, &id, NULL);

      /* A contract whose predicate never parsed -- one the shadow check
	 error-marked, say -- has no comment to copy, and copy_node does not
	 take NULL.  */
      if (CONTRACT_COMMENT (c))
	CONTRACT_COMMENT (c) = copy_node (CONTRACT_COMMENT (c));

      copies.quick_push (c);
    }

  if (copies.is_empty ())
    return NULL_TREE;

  tree new_contracts = make_tree_vec (copies.length ());
  for (unsigned ix = 0; ix < copies.length (); ix++)
    TREE_VEC_ELT (new_contracts, ix) = copies[ix];
  return new_contracts;
}

/* Returns a copy of FNDECL contracts. This is used when emitting a contract.
 If we were to emit the original contract tree, any folding of the contract
 condition would affect the original contract too. The original contract
 tree needs to be preserved in case it is used to apply to a different
 function (for inheritance or wrapping reasons). */

static tree
copy_contracts (tree fndecl, contract_match_kind remap_kind = cmk_all)
{
  tree contracts = get_fn_contract_specifiers (fndecl);
  return copy_contracts_list (contracts, fndecl, remap_kind);
}

/* Add the contract statement CONTRACT to the current block if valid.  */

static bool
emit_contract_statement (tree contract)
{
  /* Only add valid contracts.  */
  if (contract == error_mark_node
      || CONTRACT_CONDITION (contract) == error_mark_node)
    return false;

  add_stmt (contract);
  return true;
}

/* Forward declarations.  */
static tree build_quick_enforce_reaction (location_t);
static tree build_contract_data_block_address (tree);
static tree declare_cxa_entry_point (contract_assertion_kind,
				     contract_evaluation_semantic, int);

/* The state kept for postcondition captures (P3098).  */

/* Map from postcondition contract tree -> initialized flag VAR_DECL.
   Populated by the inline interleaved emission path, queried by the
   postcondition emission path to gate predicate evaluation.
   Cleared per function.  */
static hash_map<tree, tree> *postcondition_capture_flags;

/* Map from a postcondition capture VAR_DECL to its initializer, saved just
   before the inline capture-init emission destructively clears DECL_INITIAL.
   A later deep-copy of the contract (e.g. for a virtual function's wrapper,
   which is emitted after the base function has already run and cleared the
   shared capture var) recovers the initializer from here -- otherwise the
   wrapper's captures would be left uninitialized (P3097 x P3098).  GC-managed
   because the saved initializer trees must survive to wrapper-emission
   time.  */
static GTY(()) hash_map<tree, tree, pch_traits> *postcondition_capture_inits;

/* Map from postcondition contract -> capture-state RECORD_TYPE.
   Cached because the same type must be shared between __pre_fn/__post_fn
   signatures and the call site.  */
static GTY(()) hash_map<tree, tree> *postcondition_capture_struct_types;

/* Map from postcondition contract -> struct local VAR_DECL at the call site.
   Used by outlined mode to share struct locals between
   add_pre_condition_fn_call and add_post_condition_fn_call.  Cleared per
   function.  */
static hash_map<tree, tree> *outlined_capture_struct_locals;

/* A postcondition with captures whose semantic a P3595 dynamic selector
   chooses at run time chooses it once, where the captures are initialized:
   the ignore semantic has no effect at all, so the capture initializers must
   not run for it ([basic.contract.eval]), and the postcondition's own check
   then dispatches on the semantic chosen there instead of calling the
   selector again.  The selector call can only be built during
   genericization (build_contract_check), so the two statements are recorded
   here, keyed by the statement node emitted.  The value is a TREE_LIST whose
   TREE_VALUE is the uint16 lvalue holding the chosen semantic; its
   TREE_PURPOSE is integer_zero_node for the statement that stores the choice
   there and integer_one_node for the check that dispatches on it.  */

static GTY(()) hash_map<tree, tree> *dynamic_selection_sites;

/* Per function, the uint16 variable holding the semantic chosen for each
   dynamically selected postcondition with captures, keyed like
   postcondition_capture_flags.  */

static hash_map<tree, tree> *postcondition_capture_selections;

/* Record STMT, a copy of a contract statement, as a dynamic selection site
   for the semantic held in DEST: the statement that chooses it if SELECT_P,
   and otherwise the check that dispatches on it.  */

static void
note_dynamic_selection_site (tree stmt, bool select_p, tree dest)
{
  hash_map_maybe_create<hm_ggc> (dynamic_selection_sites);
  dynamic_selection_sites->put
    (stmt, build_tree_list (select_p ? integer_zero_node : integer_one_node,
			    dest));
}

/* The condition under which the check of a dynamically selected
   postcondition with captures runs: its captures were initialized (FLAG),
   or the selection held in SEL was invalid, which the check reports as an
   enforced violation without reading any capture.  */

static tree
dynamic_selection_runs_check (tree flag, tree sel)
{
  return build2 (TRUTH_OR_EXPR, boolean_type_node, flag,
		 build2 (EQ_EXPR, boolean_type_node, sel,
			 build_int_cst (TREE_TYPE (sel), CES_INVALID)));
}

/* Return the statement that chooses the semantic of CONTRACT, a dynamically
   selected postcondition, into DEST.  It is a copy of CONTRACT whose
   predicate is true, so that anything reading it as a contract (constant
   evaluation in particular) finds nothing to check.  */

static tree
build_dynamic_selection_stmt (tree contract, tree dest)
{
  tree stmt = copy_node (contract);
  CONTRACT_CONDITION (stmt) = boolean_true_node;
  note_dynamic_selection_site (stmt, /*select_p=*/true, dest);
  return stmt;
}

/* The condition under which the captures of a dynamically selected
   postcondition are initialized: the semantic chosen into SEL is one that
   evaluates the postcondition.  Ignore (and assume, which dispatch maps to
   ignore) does not, and an invalid choice is reported by the postcondition
   check itself.  */

static tree
dynamic_selection_initializes_captures (tree sel)
{
  tree type = TREE_TYPE (sel);
  return build2 (TRUTH_ANDIF_EXPR, boolean_type_node,
		 build2 (NE_EXPR, boolean_type_node, sel,
			 build_int_cst (type, CES_IGNORE)),
		 build2 (NE_EXPR, boolean_type_node, sel,
			 build_int_cst (type, CES_INVALID)));
}

/* The initializer of the postcondition capture VAR, recovered from
   postcondition_capture_inits once emitting the definition has cleared it:
   a redeclaration after the definition compares against it.  */

static tree
postcondition_capture_initializer (tree var)
{
  if (!DECL_INITIAL (var) && postcondition_capture_inits)
    if (tree *saved = postcondition_capture_inits->get (var))
      return *saved;
  return DECL_INITIAL (var);
}

/* Map from a guarded function -> the local standing in for its returned
   object in that function's inline postcondition checks.  A function is
   absent when its checks read DECL_RESULT directly; see
   postcondition_needs_retval_temp_p for when a temporary is needed.

   Populated by apply_postconditions and read by remap_retval, which does not
   run until genericization -- emit_contract_statement only queues the
   contract -- so this cannot be a per-function transient.  GC-managed for
   the same reason.  */

static GTY(()) hash_map<tree, tree, pch_traits> *postcondition_retval_temps;

/* Emit the initialization of one postcondition capture (P3098).  TARGET is
   the capture object -- a VAR_DECL on the inline path, a capture-struct
   member on the outlined one -- and INIT its initializer.

   A capture is a local variable copy-initialized from its initializer
   ([dcl.contract.capture]: the capture's initializer is a const lvalue
   denoting the parameter, or the init-capture's own initializer), so a class
   type has to go through build_aggr_init.  A bare INIT_EXPR performs no
   overload resolution: it bit-copies the object, and the capture's
   destructor then runs on a copy that no constructor ever made.  */

static void
emit_postcondition_capture_init (tree target, tree init)
{
  tree type = TREE_TYPE (target);

  if (type_build_ctor_call (type) || CLASS_TYPE_P (type))
    /* LOOKUP_ONLYCONVERTING: this is the `= init' form, so an explicit
       constructor is not a candidate.  */
    finish_expr_stmt (build_aggr_init (target, init, LOOKUP_ONLYCONVERTING,
				       tf_warning_or_error));
  else
    finish_expr_stmt (cp_build_init_expr (target, init));
}

/* Return false, after diagnosing it at LOC, if a capture of TYPE cannot be
   copy-initialized from INIT as emit_postcondition_capture_init would.
   Checked where the capture is parsed, so that it is diagnosed on every
   declaration and at the capture, not only where a body is emitted.  */

bool
postcondition_capture_init_ok_p (tree type, tree init, location_t loc)
{
  if (!CLASS_TYPE_P (type)
      || dependent_type_p (type)
      || type_dependent_expression_p (init))
    return true;
  iloc_sentinel ils (loc);
  if (!complete_type_or_else (type, NULL_TREE))
    return false;
  cp_unevaluated u;
  releasing_vec args (make_tree_vector_single (init));
  tree call = build_special_member_call (NULL_TREE, complete_ctor_identifier,
					 &args, type, LOOKUP_ONLYCONVERTING
					 | LOOKUP_NORMAL, tf_warning_or_error);
  return call != error_mark_node;
}

/* Return the type of a postcondition's init-capture pack (P3098) whose
   element type is TYPE and whose initializer expands PACKS.  TYPE need not
   contain a pack itself (e.g. [...x = sizeof (args)]), so when it does not,
   build the expansion directly, as add_capture does for a lambda's
   init-capture pack.  */

static tree
make_init_capture_pack_type (tree type, tree packs)
{
  tree exp = make_pack_expansion (type, tf_none);
  if (exp == error_mark_node)
    {
      exp = cxx_make_type (TYPE_PACK_EXPANSION);
      PACK_EXPANSION_PATTERN (exp) = type;
      SET_TYPE_STRUCTURAL_EQUALITY (exp);
      PACK_EXPANSION_LOCAL_P (exp) = local_bindings_p ();
    }
  PACK_EXPANSION_PARAMETER_PACKS (exp) = packs;
  PACK_EXPANSION_AUTO_P (exp) = true;
  return exp;
}

/* Deduce the type of a postcondition's capture (P3098) from its initializer
   *INIT -- the parameter for a plain capture, the parsed initializer for an
   init-capture (INIT_CAPTURE_P) -- as for a lambda's capture by copy.
   Top-level const is stripped: the initializer was looked up in the contract
   scope, where variables are const-ified, but captures are local to the
   assertion and are not (P3098 4.4.1).  For a pack capture (IS_PACK), wrap
   *INIT and the type in pack expansions.  For any other capture, diagnose at
   LOC an initializer that names a pack unexpanded (`[xs]' for a parameter
   pack XS wants `[...xs]', as for a lambda) or that the capture cannot be
   copy-initialized from.

   Return the type, or NULL_TREE if the capture is ill-formed; then set
   *BAD_TYPE to the type deduced for it, or to error_mark_node if none was.  */

tree
postcondition_capture_type (tree *init, bool is_pack, bool init_capture_p,
			    location_t loc, tree *bad_type)
{
  *bad_type = error_mark_node;
  tree type = unlowered_expr_type (*init);
  if (type == error_mark_node)
    return NULL_TREE;
  if (!type || TREE_CODE (type) == DEPENDENT_OPERATOR_TYPE)
    /* The initializer is type-dependent with no deducible type yet (e.g. a
       member access on a dependent object, where unlowered_expr_type is null,
       or an operator expression, whose type is the placeholder carrying its
       operator lookup -- which tsubst cannot substitute as a type).  Defer
       via decltype so the capture carries a dependent type that tsubst
       resolves at instantiation; the const-ification exemption is reapplied
       there.  */
    type = finish_decltype_type (*init, /*id_or_member=*/true,
				 tf_warning_or_error);
  else
    {
      /* A plain pack capture's parameter type is already a
	 TYPE_PACK_EXPANSION: take its pattern before re-wrapping.  */
      if (is_pack && PACK_EXPANSION_P (type))
	type = PACK_EXPANSION_PATTERN (type);
      type = cp_build_qualified_type (type,
				      cp_type_quals (type) & ~TYPE_QUAL_CONST);
    }

  if (is_pack)
    {
      *init = make_pack_expansion (*init);
      if (*init == error_mark_node)
	return NULL_TREE;
      if (init_capture_p)
	return make_init_capture_pack_type
		 (type, uses_parameter_packs (PACK_EXPANSION_PATTERN (*init)));
      type = make_pack_expansion (type);
      return type == error_mark_node ? NULL_TREE : type;
    }

  if (check_for_bare_parameter_packs (*init)
      || !postcondition_capture_init_ok_p (type, *init, loc))
    {
      *bad_type = type;
      return NULL_TREE;
    }
  return type;
}

/* True if DECL is a postcondition's capture (P3098) with no DECL_CONTEXT,
   named in its predicate.  A capture parsed with its declaration has none,
   and the late-parsed capture of a function defined outside a class has its
   context cleared by cp_parser_late_contract_condition.  Set *FN to the
   function the postcondition belongs to, the entity of the contract scope the
   capture was declared in -- null when that was parsed before the function
   was declared.  */

bool
contextless_postcondition_capture_p (tree decl, tree *fn)
{
  if (!flag_contracts
      || !processing_postcondition_predicate
      || !VAR_P (decl)
      || !DECL_ARTIFICIAL (decl)
      || DECL_CONTEXT (decl)
      || DECL_HAS_VALUE_EXPR_P (decl))
    return false;
  for (cp_binding_level *b = current_binding_level; b; b = b->level_chain)
    if (b->kind == sk_contract)
      for (tree t = b->names; t; t = DECL_CHAIN (t))
	if (t == decl)
	  {
	    *fn = b->this_entity;
	    return true;
	  }
  return false;
}

/* Within the try block around a postcondition's capture initializers, make
   the capture CAP just initialized be destroyed if a later initializer exits
   by an exception: the captures already initialized are destroyed
   ([basic.contract.eval]) before the handler reports the violation.  The
   try block's statement list closes the cleanup's scope.  */

static void
push_capture_eh_cleanup (tree cap)
{
  if (!flag_exceptions || !type_build_dtor_call (TREE_TYPE (cap)))
    return;
  /* Not build_cleanup, and tf_none -- see emit_postcondition_capture_dtors.  */
  tree dtor = cxx_maybe_build_cleanup (cap, tf_none);
  if (dtor && dtor != error_mark_node)
    push_cleanup (DECL_P (cap) ? cap : NULL_TREE, dtor, /*eh_only=*/true);
}

/* Destroy the captures of CONTRACT, a postcondition checked inline, if they
   were initialized: in reverse order, gated by the initialized flag.  */

static void
emit_capture_var_dtors (tree contract)
{
  tree captures = POSTCONDITION_CAPTURES (contract);

  /* Look up the initialized flag.  */
  tree *flag_p = postcondition_capture_flags
		 ? postcondition_capture_flags->get (contract) : NULL;

  tree if_stmt = NULL_TREE;
  if (flag_p)
    {
      if_stmt = begin_if_stmt ();
      finish_if_stmt_cond (*flag_p, if_stmt);
    }

  /* Collect captures for this postcondition and reverse.  */
  auto_vec<tree, 4> cap_vars;
  for (tree cap = captures; cap; cap = TREE_CHAIN (cap))
    cap_vars.safe_push (TREE_VALUE (cap));

  for (int j = cap_vars.length () - 1; j >= 0; j--)
    {
      tree var = cap_vars[j];
      tree type = TREE_TYPE (var);
      if (!type_build_dtor_call (type))
	continue;
      /* Not build_cleanup: type_build_dtor_call is also true for a
	 user-declared destructor that is trivial, for which
	 cxx_maybe_build_cleanup checks access and then discards the call,
	 returning NULL_TREE -- which build_cleanup asserts against.

	 tf_none because this is codegen for a capture whose initialization
	 has already been checked and diagnosed: the destructor's own
	 accessibility is part of that check, and the dtors are emitted
	 once per exit path, so diagnosing here would repeat the error.  */
      tree dtor = cxx_maybe_build_cleanup (var, tf_none);
      if (dtor && dtor != error_mark_node)
	finish_expr_stmt (dtor);
    }

  if (if_stmt)
    {
      finish_then_clause (if_stmt);
      finish_if_stmt (if_stmt);
    }
}

/* Emit explicit destruction for postcondition captures (P3098).
   Destroys captures in reverse lexical order, gated by the initialized flag.
   Only destroys if captures were successfully constructed.  */

static void
emit_postcondition_capture_dtors (tree fndecl)
{
  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return;

  /* Collect postconditions with captures in reverse order.  */
  auto_vec<tree, 4> postconds_with_caps;
  for (tree contract : tree_vec_range (contracts))
    {
      if (!active_postcondition_with_captures_p (contract, fndecl))
	continue;
      postconds_with_caps.safe_push (contract);
    }

  /* Destroy in reverse lexical order of postconditions, and within
     each postcondition in reverse order of captures.  Gated by the
     initialized flag -- only destroy if captures were fully constructed.  */
  for (int i = postconds_with_caps.length () - 1; i >= 0; i--)
    {
      emit_capture_var_dtors (postconds_with_caps[i]);
    }
}

/* Return true if FNDECL has any active postcondition with captures.  */

static bool
has_postcondition_captures_p (tree fndecl)
{
  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return false;

  for (tree contract : tree_vec_range (contracts))
    {
      if (active_postcondition_with_captures_p (contract, fndecl))
	return true;
    }
  return false;
}

/* Build (or return cached) capture-state struct type for a postcondition
   with captures.  The struct has a bool __initialized field followed by
   union-wrapped fields for each capture (unions prevent implicit
   construction/destruction).  */

static tree
get_postcondition_capture_struct_type (tree contract)
{
  gcc_assert (POSTCONDITION_P (contract));
  gcc_assert (POSTCONDITION_CAPTURES (contract));

  tree *cached
    = hash_map_safe_get (postcondition_capture_struct_types, contract);
  if (cached)
    return *cached;

  tree captures = POSTCONDITION_CAPTURES (contract);

  tree struct_type = make_node (RECORD_TYPE);
  tree fields = NULL_TREE;
  tree *last_field = &fields;

  tree init_field = build_decl (UNKNOWN_LOCATION, FIELD_DECL,
				get_identifier ("__initialized"),
				boolean_type_node);
  DECL_CONTEXT (init_field) = struct_type;
  DECL_ARTIFICIAL (init_field) = 1;
  *last_field = init_field;
  last_field = &DECL_CHAIN (init_field);

  for (tree cap = captures; cap; cap = TREE_CHAIN (cap))
    {
      tree var = TREE_VALUE (cap);
      tree cap_type = TREE_TYPE (var);

      tree union_type = make_node (UNION_TYPE);
      tree union_member = build_decl (DECL_SOURCE_LOCATION (var), FIELD_DECL,
				      DECL_NAME (var), cap_type);
      DECL_CONTEXT (union_member) = union_type;
      TYPE_FIELDS (union_type) = union_member;
      layout_type (union_type);

      tree struct_field = build_decl (DECL_SOURCE_LOCATION (var), FIELD_DECL,
				      DECL_NAME (var), union_type);
      DECL_CONTEXT (struct_field) = struct_type;
      DECL_ARTIFICIAL (struct_field) = 1;
      *last_field = struct_field;
      last_field = &DECL_CHAIN (struct_field);
    }

  /* A dynamically selected postcondition also carries the semantic its
     selector chose (see dynamic_selection_sites).  */
  if (contract_dynamic_name (contract))
    {
      tree sem_field = build_decl (UNKNOWN_LOCATION, FIELD_DECL,
				   get_identifier ("__sem"),
				   short_unsigned_type_node);
      DECL_CONTEXT (sem_field) = struct_type;
      DECL_ARTIFICIAL (sem_field) = 1;
      *last_field = sem_field;
      last_field = &DECL_CHAIN (sem_field);
    }

  TYPE_FIELDS (struct_type) = fields;
  TYPE_ARTIFICIAL (struct_type) = 1;
  layout_type (struct_type);

  hash_map_maybe_create<hm_ggc> (postcondition_capture_struct_types);
  postcondition_capture_struct_types->put (contract, struct_type);
  return struct_type;
}

/* Declare capture-state struct local variables at the call site for
   outlined mode.  One struct per active postcondition with captures.
   Stores them in outlined_capture_struct_locals for use by
   add_pre/post_condition_fn_call and emit_outlined_capture_struct_dtors.  */

static void
declare_outlined_capture_struct_locals (tree fndecl)
{
  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return;

  unsigned idx = 0;
  for (tree contract : tree_vec_range (contracts))
    {
      if (!active_postcondition_with_captures_p (contract, fndecl))
	continue;

      tree struct_type = get_postcondition_capture_struct_type (contract);

      char buf[32];
      snprintf (buf, sizeof buf, "__captures_%u", idx++);
      tree var = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			     get_identifier (buf), struct_type);
      DECL_ARTIFICIAL (var) = 1;
      DECL_CONTEXT (var) = fndecl;
      layout_decl (var, 0);
      pushdecl (var);
      add_decl_expr (var);

      hash_map_maybe_create<hm_heap> (outlined_capture_struct_locals);
      outlined_capture_struct_locals->put (contract, var);
    }
}

/* Append capture-state struct arguments to ARGS for an outlined
   pre or post function call.  ARGS is taken by reference: pushing may
   reallocate the vector.  */

static void
append_capture_struct_args (tree fndecl, releasing_vec &args)
{
  if (!outlined_capture_struct_locals)
    return;

  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return;

  for (tree contract : tree_vec_range (contracts))
    {
      if (!active_postcondition_with_captures_p (contract, fndecl))
	continue;

      tree *struct_var_p = outlined_capture_struct_locals->get (contract);
      gcc_assert (struct_var_p);
      /* build_thunk_like_call skips conversions; we must explicitly take
	 the address for the reference parameter.  */
      vec_safe_push (args, build_address (*struct_var_p));
    }
}

/* Destroy the captures held in STRUCT_VAR, the capture-state struct of an
   outlined postcondition, if they were initialized.  */

static void
emit_capture_struct_dtors (tree struct_var)
{
  tree struct_type = TREE_TYPE (struct_var);
  tree init_field = TYPE_FIELDS (struct_type);

  tree init_ref = build3 (COMPONENT_REF, TREE_TYPE (init_field),
			  struct_var, init_field, NULL_TREE);

  tree if_stmt = begin_if_stmt ();
  finish_if_stmt_cond (init_ref, if_stmt);

  auto_vec<tree, 4> cap_fields;
  for (tree f = DECL_CHAIN (init_field); f; f = DECL_CHAIN (f))
    /* Each capture is wrapped in a union; skip the trailing __sem.  */
    if (TREE_CODE (TREE_TYPE (f)) == UNION_TYPE)
      cap_fields.safe_push (f);

  for (int j = cap_fields.length () - 1; j >= 0; j--)
    {
      tree field = cap_fields[j];
      tree union_type = TREE_TYPE (field);
      tree union_member = TYPE_FIELDS (union_type);
      tree cap_type = TREE_TYPE (union_member);

      if (!type_build_dtor_call (cap_type))
	continue;

      tree union_ref = build3 (COMPONENT_REF, union_type,
			       struct_var, field, NULL_TREE);
      tree member_ref = build3 (COMPONENT_REF, cap_type,
				union_ref, union_member, NULL_TREE);

      /* Not build_cleanup, and tf_none -- see the inline capture site.  */
      tree dtor = cxx_maybe_build_cleanup (member_ref, tf_none);
      if (dtor && dtor != error_mark_node)
	finish_expr_stmt (dtor);
    }

  finish_then_clause (if_stmt);
  finish_if_stmt (if_stmt);
}

/* Emit destruction of captures stored in outlined capture-state struct
   locals.  Used at the call site for both normal and EH cleanup paths.
   Destroys captures in reverse lexical order of postconditions, and within
   each postcondition in reverse order of captures.  */

static void
emit_outlined_capture_struct_dtors (tree fndecl)
{
  if (!outlined_capture_struct_locals)
    return;

  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return;

  auto_vec<tree, 4> postconds_with_caps;
  for (tree contract : tree_vec_range (contracts))
    {
      if (!active_postcondition_with_captures_p (contract, fndecl))
	continue;
      postconds_with_caps.safe_push (contract);
    }

  for (int i = postconds_with_caps.length () - 1; i >= 0; i--)
    {
      tree contract = postconds_with_caps[i];
      tree *struct_var_p = outlined_capture_struct_locals->get (contract);
      if (!struct_var_p)
	continue;

      emit_capture_struct_dtors (*struct_var_p);
    }
}

/* Add a call or a direct evaluation of the pre checks.  */

static void
apply_preconditions (tree fndecl)
{
  if (flag_contract_checks_outlined && DECL_PRE_FN (fndecl))
    add_pre_condition_fn_call (fndecl);
  else
  {
    if (tree contract_copy = copy_contracts (fndecl, cmk_pre))
      for (tree contract : tree_vec_range (contract_copy))
	emit_contract_statement (contract);
  }
}

/* Emit preconditions and postcondition capture inits in lexical order (P3098).
   This walks all contracts and emits:
   - For preconditions: the check (inline)
   - For postconditions with captures: the capture initialization
   - For postconditions without captures: nothing (handled at postcond site)
   It is used for inline emission when captures are present.  */

/* Emit the reaction to a capture initializer of CONTRACT, a postcondition
   of FNDECL, exiting via an exception: a post_capture violation reported
   with the postcondition's semantic.  SEL, if non-null, holds the semantic a
   dynamic selector chose, and the reaction dispatches on it; otherwise the
   semantic is the one resolved at compile time.  */

static void
emit_capture_exception_reaction (tree contract, tree fndecl, tree sel)
{
  auto react = [&] (contract_evaluation_semantic sem)
  {
    if (sem == CES_QUICK)
      {
	finish_expr_stmt
	  (build_quick_enforce_reaction (EXPR_LOCATION (contract)));
	return;
      }
    tree data_addr = build_contract_data_block_address (contract);
    tree ep = declare_cxa_entry_point (CAK_POST_CAPTURE, sem,
				       CDM_EVAL_EXCEPTION);
    finish_expr_stmt (build_call_n (ep, 1, data_addr));
  };

  if (!sel)
    {
      react (ensure_evaluation_semantic (contract, fndecl, false));
      return;
    }

  /* The captures were initialized, so SEL holds a semantic that evaluates
     the postcondition; any other value cannot reach here.  */
  auto_vec<tree, 5> ifs;
  for (int s : dynamic_dispatch_semantics ())
    {
      if (contract_semantic_emits_no_check ((contract_evaluation_semantic) s))
	continue;
      tree if_stmt = begin_if_stmt ();
      finish_if_stmt_cond (build2 (EQ_EXPR, boolean_type_node, sel,
				   build_int_cst (TREE_TYPE (sel), s)),
			   if_stmt);
      react ((contract_evaluation_semantic) s);
      finish_then_clause (if_stmt);
      begin_else_clause (if_stmt);
      ifs.safe_push (if_stmt);
    }
  for (int i = ifs.length () - 1; i >= 0; i--)
    {
      finish_else_clause (ifs[i]);
      finish_if_stmt (ifs[i]);
    }
}

static void
emit_preconditions_and_capture_inits_interleaved (tree fndecl)
{
  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return;

  /* We need copies of preconditions for emission (to avoid modifying
     originals during folding).  Walk originals for ordering, emit copies
     for preconditions.  */
  tree pre_copies = copy_contracts (fndecl, cmk_pre);
  int pre_ix = 0;
  int pre_len = pre_copies ? TREE_VEC_LENGTH (pre_copies) : 0;
  /* Numbers the capture flags and selectors, which are local to FNDECL.  */
  unsigned cap_ix = 0;

  for (tree contract : tree_vec_range (contracts))
    {
      if (TREE_CODE (contract) == PRECONDITION_STMT)
	{
	  /* Emit this precondition (from the copy vector).  */
	  gcc_checking_assert (pre_ix < pre_len);
	  emit_contract_statement (TREE_VEC_ELT (pre_copies, pre_ix++));
	}
      else if (POSTCONDITION_P (contract))
	{
	  /* Emit capture initialization for this postcondition.  The
	     TREE_CODE test above is already the postcondition half of the
	     predicate; it is re-tested here rather than restructured because
	     this arm is one branch of a precondition/postcondition walk.  */
	  if (!active_postcondition_with_captures_p (contract, fndecl))
	    continue;
	  tree captures = POSTCONDITION_CAPTURES (contract);

	  /* Create the initialized flag.  */
	  unsigned ix = cap_ix++;
	  char buf[40];
	  snprintf (buf, sizeof buf, "__captures_initialized_%u", ix);
	  tree flag = build_decl (UNKNOWN_LOCATION, VAR_DECL,
				  get_identifier (buf), boolean_type_node);
	  DECL_ARTIFICIAL (flag) = 1;
	  DECL_CONTEXT (flag) = fndecl;
	  layout_decl (flag, 0);
	  pushdecl (flag);
	  add_decl_expr (flag);
	  finish_expr_stmt (cp_build_init_expr (flag, boolean_false_node));

	  hash_map_maybe_create<hm_heap> (postcondition_capture_flags);
	  postcondition_capture_flags->put (contract, flag);

	  /* Declare capture variables.  */
	  for (tree cap = captures; cap; cap = TREE_CHAIN (cap))
	    {
	      tree var = TREE_VALUE (cap);
	      DECL_CONTEXT (var) = fndecl;
	      layout_decl (var, 0);
	      pushdecl (var);
	      add_decl_expr (var);
	    }

	  /* A dynamically selected postcondition chooses its semantic here,
	     and initializes the captures only if it is to be evaluated.  */
	  tree sel = NULL_TREE;
	  tree gate = NULL_TREE;
	  if (contract_dynamic_name (contract))
	    {
	      snprintf (buf, sizeof buf, "__captures_semantic_%u", ix);
	      sel = build_decl (UNKNOWN_LOCATION, VAR_DECL,
				get_identifier (buf), short_unsigned_type_node);
	      DECL_ARTIFICIAL (sel) = 1;
	      DECL_CONTEXT (sel) = fndecl;
	      layout_decl (sel, 0);
	      pushdecl (sel);
	      add_decl_expr (sel);
	      add_stmt (build_dynamic_selection_stmt (contract, sel));
	      hash_map_maybe_create<hm_heap> (postcondition_capture_selections);
	      postcondition_capture_selections->put (contract, sel);
	      gate = begin_if_stmt ();
	      finish_if_stmt_cond (dynamic_selection_initializes_captures (sel),
				   gate);
	    }

	  /* The try/catch exists only to turn a throwing capture initializer
	     into a post_capture violation.  With -fno-exceptions nothing can
	     throw and there is nothing to catch, so building it would just
	     reach doing_eh() in cp/except.cc and error out; emit the bare
	     initializations instead.  */
	  tree try_block = NULL_TREE;
	  if (flag_exceptions)
	    try_block = begin_try_block ();

	  for (tree cap = captures; cap; cap = TREE_CHAIN (cap))
	    {
	      tree var = TREE_VALUE (cap);
	      tree init = DECL_INITIAL (var);
	      if (!init || init == error_mark_node)
		continue;
	      /* Preserve the initializer before the emission below clears it,
		 so a later deep-copy of this contract to a virtual function's
		 wrapper can recover it (the wrapper shares this capture var and
		 is emitted after us).  */
	      hash_map_maybe_create<hm_ggc> (postcondition_capture_inits);
	      postcondition_capture_inits->put (var, init);
	      emit_postcondition_capture_init (var, init);
	      DECL_INITIAL (var) = NULL_TREE;
	      push_capture_eh_cleanup (var);
	    }
	  finish_expr_stmt (cp_build_init_expr (flag, boolean_true_node));

	  if (flag_exceptions)
	    {
	      finish_try_block (try_block);

	      /* Catch handler.  */
	      tree handler = begin_handler ();
	      finish_handler_parms (NULL_TREE, handler);
	      emit_capture_exception_reaction (contract, fndecl, sel);
	      finish_handler (handler);
	      finish_handler_sequence (try_block);
	    }

	  if (gate)
	    {
	      finish_then_clause (gate);
	      finish_if_stmt (gate);
	    }

	  /* Until the cleanup region maybe_apply_function_contracts builds
	     around the body begins, an exception from a later precondition or
	     capture must destroy these captures too.  */
	  if (flag_exceptions)
	    {
	      tree cleanup = push_stmt_list ();
	      emit_capture_var_dtors (contract);
	      push_cleanup (NULL_TREE, pop_stmt_list (cleanup),
			    /*eh_only=*/true);
	    }
	}
    }
}

/* True if the inline postcondition checks for FNDECL have to read the
   returned object through a temporary instead of through DECL_RESULT.

   Aliasing the result binding onto DECL_RESULT is the same manoeuvre as the
   NRVO, and it needs the same guard the NRVO has: want_nrvo_p (cp/typeck.cc)
   only aliases when aggregate_value_p says the result genuinely lives in
   memory.  When it comes back in a register there is nothing to take the
   address of -- expand_function_start gives DECL_RESULT a pseudo and never
   consults TREE_ADDRESSABLE -- so a predicate needing that address dies on
   the MEM_P assertion in expand_expr_addr_expr_1 (PR c++/125574).

   Introducing a temporary here is what [dcl.contract.res] describes: its
   Example 2 has the check "can fail if the implementation introduces a
   temporary for the return value" precisely for a register-returned type,
   and succeed for one returned in memory.  The outlined checking mode has
   always done this -- __post_fn takes the result as a by-value parameter,
   which is why -fcontract-checks-outlined never saw the ICE.

   Only an addressable result binding needs it; the ordinary
   post(r: r > 0) goes on reading DECL_RESULT directly.  */

static bool
postcondition_needs_retval_temp_p (tree fndecl)
{
  tree restype = TREE_TYPE (TREE_TYPE (fndecl));
  if (VOID_TYPE_P (restype) || !DECL_RESULT (fndecl))
    return false;

  /* Returned in memory: DECL_RESULT is addressable and, per Example 2, is
     the object the caller sees -- do not copy it.  */
  if (aggregate_value_p (restype, fndecl))
    return false;

  /* A scalar result needs a home too, and for a sharper reason than an
     aggregate does.  It is a gimple register, so gimplify_addr_expr spills it
     to a temporary of its own every time a predicate takes its address -- and
     a predicate may do so more than once:

       int f () post (r : rec (&r) && rec2 (addr_via_ref (r))) ...

     gives two spills and therefore two addresses for one result binding,
     inside a single evaluation of a single predicate.  [dcl.contract.res]/1
     binds the result name to one object; nothing permits two, and the
     predicate's value comes out wrong as a result (PR c++/112794).  Give it one
     home so every use in the predicate names the same object.

     This does move a direct `const_cast<int&>(r)++' off DECL_RESULT, which
     g++.dg/contracts/cpp26/expr.prim.id.unqual.p7-4.C pins -- so the caller
     copies the temporary back once the checks are done, keeping the mutation
     observable.  The copy-back is therefore not optional: a stand-in copied
     into but never copied back loses that mutation, which is worse than
     giving the result no home at all.  */

  /* The copy below is a bare INIT_EXPR, so only take this path for a type
     it is a correct initialization for.  Under the Itanium ABI anything
     less is returned in memory and has already been excluded above.  */
  if (!trivially_copyable_p (restype))
    return false;

  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return false;

  for (tree contract : tree_vec_range (contracts))
    if (POSTCONDITION_P (contract))
      {
	tree result = POSTCONDITION_IDENTIFIER (contract);
	if (result && DECL_P (result) && TREE_ADDRESSABLE (result))
	  return true;
      }

  return false;
}

/* Add a call or a direct evaluation of the post checks.
   For postconditions with captures, gate the predicate check on the
   initialized flag (set by the interleaved emission path).  */

static void
apply_postconditions (tree fndecl)
{
  if (flag_contract_checks_outlined && DECL_POST_FN (fndecl))
    {
      /* D4405: the override shadows' regions stay in FNDECL.  */
      add_override_post_obligations (NULL_TREE);
      add_post_condition_fn_call (fndecl);
      return;
    }

  /* Give the checks an addressable stand-in for the returned object when
     DECL_RESULT cannot supply one; remap_retval picks this up.  */
  if (postcondition_needs_retval_temp_p (fndecl))
    {
      tree restype = TREE_TYPE (TREE_TYPE (fndecl));
      tree tmp = build_decl (DECL_SOURCE_LOCATION (fndecl), VAR_DECL,
			     get_identifier ("__contract_retval"), restype);
      DECL_ARTIFICIAL (tmp) = 1;
      DECL_IGNORED_P (tmp) = 1;
      DECL_CONTEXT (tmp) = fndecl;
      TREE_ADDRESSABLE (tmp) = 1;
      layout_decl (tmp, 0);
      pushdecl (tmp);
      add_decl_expr (tmp);
      finish_expr_stmt (cp_build_init_expr (tmp, DECL_RESULT (fndecl)));
      hash_map_maybe_create<hm_ggc> (postcondition_retval_temps);
      postcondition_retval_temps->put (fndecl, tmp);
    }

  /* Walk original and copy in lockstep so we can look up capture flags
     by original contract tree.  For postconditions with captures, we
     skip copy_contracts (which can remap capture VAR_DECL references)
     and emit the original contract directly.  */
  tree contract_copy = copy_contracts (fndecl, cmk_post);
  if (!contract_copy)
    return;

  tree orig_contracts = get_fn_contract_specifiers (fndecl);
  int orig_ix = 0;
  int orig_len = orig_contracts ? TREE_VEC_LENGTH (orig_contracts) : 0;

  for (tree contract : tree_vec_range (contract_copy))
    {
      /* Advance through the originals to the postcondition this copy came
	 from; CONTRACT_COPY holds only postconditions, in source order.  */
      tree orig_contract = NULL_TREE;
      while (orig_ix < orig_len)
	{
	  tree orig = TREE_VEC_ELT (orig_contracts, orig_ix++);
	  if (POSTCONDITION_P (orig))
	    {
	      orig_contract = orig;
	      break;
	    }
	}

      /* D4405: its obligations in the override shadows, ahead of it.  */
      if (orig_contract)
	add_override_post_obligations (orig_contract);

      /* If this postcondition has captures, gate on the initialized flag.  */
      tree *flag_p = NULL;
      if (orig_contract && POSTCONDITION_CAPTURES (orig_contract)
	  && postcondition_capture_flags)
	flag_p = postcondition_capture_flags->get (orig_contract);

      if (flag_p)
	{
	  /* Start from the ORIGINAL contract (not the copy) for postconditions
	     with captures, as copy_contracts can remap capture VAR_DECL refs.
	     But genericization rewrites the check's condition in place, and the
	     original is what a later redeclaration is compared against, so emit
	     a copy with a condition of its own: unshare_expr copies the
	     expression but keeps the capture VAR_DECLs.  A dynamically selected
	     postcondition's check is keyed by that copy, and dispatches on the
	     semantic chosen where its captures were initialized.  */
	  tree check = copy_node (orig_contract);
	  CONTRACT_CONDITION (check)
	    = unshare_expr (CONTRACT_CONDITION (orig_contract));
	  tree cond = *flag_p;
	  if (tree *sel_p = hash_map_safe_get (postcondition_capture_selections,
					       orig_contract))
	    {
	      note_dynamic_selection_site (check, /*select_p=*/false, *sel_p);
	      cond = dynamic_selection_runs_check (cond, *sel_p);
	    }
	  tree if_stmt = begin_if_stmt ();
	  finish_if_stmt_cond (cond, if_stmt);
	  emit_contract_statement (check);
	  finish_then_clause (if_stmt);
	  finish_if_stmt (if_stmt);
	}
      else
	emit_contract_statement (contract);
    }

  /* If the checks ran against a stand-in for the returned object, copy it
     back: having evaluated the predicates we owe their effects to the
     caller, and the stand-in is where those effects landed.  */
  if (postcondition_retval_temps)
    if (tree *tmp = postcondition_retval_temps->get (fndecl))
      finish_expr_stmt (build2 (MODIFY_EXPR, TREE_TYPE (*tmp),
				DECL_RESULT (fndecl), *tmp));
}

/* Wrap STMTS -- the postcondition checks of FNDECL -- in a cleanup that
   destroys the returned object if evaluating them exits via an exception,
   which a violation handler that throws will do.

   By the time the checks run the returned object has been initialized:
   [stmt.return]/5 sequences postcondition evaluation after the copy-
   initialization of the result and after the destruction of local variables.
   Unwinding past it without running its destructor leaks an object the
   program can no longer reach.

   This is the FALLBACK, not the usual path.  An ordinary function gets one
   sentinel-guarded cleanup spliced around the whole contracts block, covering
   the body and the checks together (see maybe_apply_function_contracts and
   maybe_splice_retval_cleanup); this wrapper is for the case that cleanup
   cannot reach -- a coroutine ramp, whose transform clears throwing_cleanup
   because it manages its own cleanups, so no sentinel is ever built.  The
   caller picks between them, and only one of the two is ever emitted, which
   is what keeps the object from being destroyed twice.

   No sentinel guard here, unlike maybe_splice_retval_cleanup: this region is
   reached only on the normal-completion path of the body, where the returned
   object necessarily exists, so there is nothing to test.

   NOTE this is deliberately more than the standard currently requires.
   [except.ctor]/2 destroys the returned object only for an exception thrown
   "during the destruction of temporaries or local variables for a return
   statement", and does not mention contract assertions; [basic.contract.eval]
   says a throwing handler behaves "as if the function body exits via that
   same exception", which describes a state where the result object was never
   initialized -- not the state we are actually in.  So nothing obliges us to
   run the destructor here, but leaking the object is not defensible, and the
   wording alone is no reason to change this back to a leak.  */

static tree
wrap_postconditions_in_retval_cleanup (tree fndecl, tree stmts)
{
  if (!flag_exceptions || !stmts)
    return stmts;

  tree retval = DECL_RESULT (fndecl);
  if (!retval
      || VOID_TYPE_P (TREE_TYPE (retval))
      || !TYPE_HAS_NONTRIVIAL_DESTRUCTOR (TREE_TYPE (retval)))
    return stmts;

  tree dtor = build_cleanup (retval);
  if (!dtor || dtor == error_mark_node)
    return stmts;

  tree cleanup = build_stmt (UNKNOWN_LOCATION, CLEANUP_STMT,
			     stmts, dtor, retval);
  CLEANUP_EH_ONLY (cleanup) = true;

  tree list = NULL_TREE;
  append_to_statement_list_force (cleanup, &list);
  return list;
}

/* Add contract handling to the function in FNDECL.

   When we have only pre-conditions, this simply prepends a call (or a direct
   evaluation, for cdtors) to the existing function body.

   When we have post conditions we build a try-finally block.
   If the function might throw then the handler in the try-finally is an
   EH_ELSE expression, where the post condition check is applied to the
   non-exceptional path, and an empty statement is added to the EH path.  If
   the function has a non-throwing eh spec, then the handler is simply the
   post-condition checker.  */

void
maybe_apply_function_contracts (tree fndecl)
{
  /* D4405: the override shadows (begin_override_shadows), by themselves
     when FNDECL has no active postconditions (place_override_shadows_alone),
     and otherwise their regions at entry ahead of everything else below and
     the obligations of the postconditions with the postconditions' checks
     (add_override_base_post_obligations).  */
  override_shadows *overrides = begin_override_shadows (fndecl);
  if (overrides
      && (!handle_contracts_p (fndecl) || !has_active_postconditions (fndecl)))
    {
      place_override_shadows_alone (overrides);
      return;
    }

  if (!handle_contracts_p (fndecl))
    /* We did nothing and the original function body statement list will be
       popped by our caller.  */
    return;

  bool do_pre = has_active_preconditions (fndecl);
  bool do_post = has_active_postconditions (fndecl);
  if (!do_pre && !do_post)
    return;

  /* If the function is noexcept, the user's written body will normally be
     wrapped in a MUST_NOT_THROW expression.  In that case we leave the
     MUST_NOT_THROW in place and do our replacement inside it.

     "Normally", not "always": the wrapper comes from begin_eh_spec_block,
     which use_eh_spec_block gates on flag_enforce_eh_specs among other
     things.  Under -fno-enforce-eh-specs a noexcept function has no wrapper
     at all, so keying off TYPE_NOEXCEPT_P alone read whatever the body did
     start with as a MUST_NOT_THROW_EXPR and took its first operand -- a
     checking-assert in a checking build, and a segfault in a release one
     (PR c++/127173).  Test the body for the wrapper instead of predicting it
     from the exception specification; that also covers the other reasons
     use_eh_spec_block can decline, such as a cloned or implicitly-generated
     function.  */
  tree fnbody;
  tree m_n_t_expr = (TYPE_NOEXCEPT_P (TREE_TYPE (fndecl))
		     ? expr_first (DECL_SAVED_TREE (fndecl)) : NULL_TREE);
  if (m_n_t_expr && m_n_t_expr != error_mark_node
      && TREE_CODE (m_n_t_expr) == MUST_NOT_THROW_EXPR)
    {
      fnbody = TREE_OPERAND (m_n_t_expr, 0);
      TREE_OPERAND (m_n_t_expr, 0) = push_stmt_list ();
    }
  else
    {
      fnbody = DECL_SAVED_TREE (fndecl);
      DECL_SAVED_TREE (fndecl) = push_stmt_list ();
    }

  /* If we have a lambda with captures, ensure that those captures are in-
     scope for pre and post conditions.

     The BIND_EXPR that declares the capture proxies does not always arrive
     as the body itself: it can be wrapped in a STATEMENT_LIST holding
     nothing else.  Matching only the bare BIND_EXPR left the proxies out of
     scope, so the precondition ended up as a *preceding sibling* of the
     BIND_EXPR declaring them and gimplify_var_or_parm_decl asserted on a
     proxy it had not seen in any bind (PR c++/126038).  Postconditions were
     unaffected -- they are emitted after the body, by which point the
     gimplifier has walked that bind.  Look through the wrapper.  */
  if (LAMBDA_FUNCTION_P (fndecl))
    {
      tree bind = fnbody;
      if (TREE_CODE (bind) == STATEMENT_LIST)
	{
	  tree_stmt_iterator i = tsi_start (bind);
	  if (!tsi_end_p (i))
	    {
	      tree only = tsi_stmt (i);
	      tsi_next (&i);
	      if (tsi_end_p (i))
		bind = only;
	    }
	}

      if (TREE_CODE (bind) == BIND_EXPR)
	{
	  tree extract = BIND_EXPR_BODY (bind);
	  BIND_EXPR_BODY (bind) = NULL_TREE;
	  add_stmt (bind);
	  BIND_EXPR_BODY (bind) = push_stmt_list ();
	  fnbody = extract;
	}
    }

  /* Now add the pre and post conditions to the existing function body.
     This copies the approach used for function try blocks.  */

  /* We are called from finish_function with the sk_function_parms level
     current, so do_poplevel sees that same level again when it finishes the
     artificial block below -- exactly the test maybe_splice_retval_cleanup
     uses to recognise the function body.  That second visit is not a problem
     to be suppressed but the one we want: start_function_contracts asked for
     the body's splice to be deferred, so the single return-value cleanup is
     spliced here instead, around the body AND the postcondition checks.  A
     check that throws then destroys the returned object exactly once, by the
     same sentinel-guarded cleanup that covers the body.

     UNIFIED_RETVAL_CLEANUP is false when there is no sentinel to splice: a
     coroutine ramp, whose transform clears throwing_cleanup because it
     manages its own cleanups, and any function whose postconditions cannot
     throw.  Those fall back to wrapping the checks directly, below.

     The constructor/destructor exclusions are not redundant:
     current_retval_sentinel is #defined to current_vtt_parm, so for a cdtor
     it reads a genuine VTT parameter and would answer "yes" to a question
     about a sentinel that does not exist.  maybe_splice_retval_cleanup bails
     on cdtors for the same reason, and the two must agree or we would emit
     neither cleanup.

     The throwing_cleanup test is what keeps coroutines on the fallback: the
     coroutine transform runs before us and clears it, so the splice below
     would take its "only using the sentinel for an NRV" exit and emit
     nothing.  Claiming the unified path there would skip the wrapper too and
     leave the ramp's return object with no cleanup at all.  Every condition
     maybe_splice_retval_cleanup will apply has to be mirrored here, or the
     two disagree and we emit either none or both.  */
  const bool unified_retval_cleanup
    = (cp_function_chain->defer_retval_cleanup
       && cp_function_chain->throwing_cleanup
       && !DECL_CONSTRUCTOR_P (fndecl)
       && !DECL_DESTRUCTOR_P (fndecl)
       && current_retval_sentinel);

  /* The fallback, for the cases the unified cleanup does not reach.  Gated on
     the same question the unified path was gated on, so postconditions that
     cannot throw -- every one of them ignore, assume, quick_enforce or a
     P4298 noexcept_ variant -- now carry no cleanup at all, where before they
     carried one that could never run.  */
  const bool wrap_checks_directly
    = !unified_retval_cleanup && postconditions_may_throw_p (fndecl);

  tree compound_stmt = begin_compound_stmt (0);
  current_binding_level->artificial = true;

  /* Do not add locations for the synthesised code.  */
  location_t loc = UNKNOWN_LOCATION;

  /* For other cases, we call a function to process the check.  */

  if (overrides)
    start_override_shadows (overrides, cur_stmt_list);

  /* If we have a pre, but not a post, then just emit that and we are done.  */
  if (!do_post)
    {
      apply_preconditions (fndecl);
      add_stmt (fnbody);
      finish_compound_stmt (compound_stmt);
      return;
    }

  /* For outlined mode with captures, declare struct locals on the caller's
     stack.  These are passed by reference to __pre_fn and __post_fn and
     destroyed inline at the call site.  */
  bool outlined_caps = (flag_contract_checks_outlined
			&& has_postcondition_captures_p (fndecl));
  if (outlined_caps)
    declare_outlined_capture_struct_locals (fndecl);

  /* Emit preconditions and capture inits.  For outlined mode, __pre_fn
     handles interleaving internally.  For inline mode with captures,
     use the interleaved version that respects lexical ordering (P3098).  */
  if (flag_contract_checks_outlined)
    apply_preconditions (fndecl);
  else if (has_postcondition_captures_p (fndecl))
    {
      /* In a statement list of its own, so that the EH-only capture
	 cleanups it pushes end here, where the region below takes over.  */
      tree region = push_stmt_list ();
      emit_preconditions_and_capture_inits_interleaved (fndecl);
      add_stmt (pop_stmt_list (region));
    }
  else if (do_pre)
    apply_preconditions (fndecl);

  tree try_fin = build_stmt (loc, TRY_FINALLY_EXPR, fnbody, NULL_TREE);
  add_stmt (try_fin);
  TREE_OPERAND (try_fin, 1) = push_stmt_list ();
  /* If we have exceptions, and a function that might throw, then add
     an EH_ELSE clause that allows the exception to propagate upwards
     without encountering the post-condition checks.  The EH path must
     still destroy postcondition captures.  */
  if (flag_exceptions && !type_noexcept_p (TREE_TYPE (fndecl)))
    {
      tree eh_else = build_stmt (loc, EH_ELSE_EXPR, NULL_TREE, NULL_TREE);
      add_stmt (eh_else);
      /* Non-exceptional path: check postconditions, then destroy captures.
	 The checks may exit via an exception (a throwing violation handler),
	 and the returned object exists by now, so they carry a cleanup that
	 destroys it.  */
      tree post_stmts = push_stmt_list ();
      apply_postconditions (fndecl);
      add_override_base_post_obligations ();
      post_stmts = pop_stmt_list (post_stmts);
      TREE_OPERAND (eh_else, 0) = push_stmt_list ();
      add_stmt (wrap_checks_directly
		? wrap_postconditions_in_retval_cleanup (fndecl, post_stmts)
		: post_stmts);
      if (outlined_caps)
	emit_outlined_capture_struct_dtors (fndecl);
      else
	emit_postcondition_capture_dtors (fndecl);
      TREE_OPERAND (eh_else, 0) = pop_stmt_list (TREE_OPERAND (eh_else, 0));
      /* Exceptional path: destroy captures without checking predicates.  */
      TREE_OPERAND (eh_else, 1) = push_stmt_list ();
      if (outlined_caps)
	emit_outlined_capture_struct_dtors (fndecl);
      else
	emit_postcondition_capture_dtors (fndecl);
      TREE_OPERAND (eh_else, 1) = pop_stmt_list (TREE_OPERAND (eh_else, 1));
    }
  else
    {
      tree post_stmts = push_stmt_list ();
      apply_postconditions (fndecl);
      add_override_base_post_obligations ();
      post_stmts = pop_stmt_list (post_stmts);
      add_stmt (wrap_checks_directly
		? wrap_postconditions_in_retval_cleanup (fndecl, post_stmts)
		: post_stmts);
      if (outlined_caps)
	emit_outlined_capture_struct_dtors (fndecl);
      else
	emit_postcondition_capture_dtors (fndecl);
    }
  TREE_OPERAND (try_fin, 1) = pop_stmt_list (TREE_OPERAND (try_fin, 1));

  /* Hand the splice back to do_poplevel: everything the cleanup must cover is
     now in this block, so closing it is what puts the cleanup in the right
     place.  */
  cp_function_chain->defer_retval_cleanup = false;
  finish_compound_stmt (compound_stmt);

  if (outlined_capture_struct_locals)
    {
      delete outlined_capture_struct_locals;
      outlined_capture_struct_locals = NULL;
    }
  if (postcondition_capture_flags)
    {
      delete postcondition_capture_flags;
      postcondition_capture_flags = NULL;
    }
  if (postcondition_capture_selections)
    {
      delete postcondition_capture_selections;
      postcondition_capture_selections = NULL;
    }
}

/* SP is a parameter (or result placeholder) of the guarded function and DP
   the corresponding parameter of an outlined checking function.  When the
   checks are outlined those parameters are passed by reference so that a
   predicate sees the guarded function's own objects rather than copies (see
   build_contract_condition_function).  Uses of SP inside the predicate are
   value uses, so map them to *DP rather than to DP itself.  When DP is not a
   reference -- every other user of remap_contract, notably the wrapper
   copies -- this is the identity.  */

static tree
remap_as_value (tree sp, tree dp)
{
  if (TYPE_REF_P (TREE_TYPE (dp)) && !TYPE_REF_P (TREE_TYPE (sp)))
    return convert_from_reference (dp);
  return dp;
}

/* Map in ID each of SRC's parameters to the corresponding parameter of DST,
   pairing them past each function's artificial parameters and stopping at
   the end of either list.  If AS_VALUE, map through remap_as_value.  Returns
   true if any mapping was inserted.  */

static bool
map_corresponding_parms (copy_body_data *id, tree src, tree dst,
			 bool as_value)
{
  bool mapped = false;
  int src_num_artificial = num_artificial_parms_for (src);
  int dst_num_artificial = num_artificial_parms_for (dst);

  for (tree sp = DECL_ARGUMENTS (src), dp = DECL_ARGUMENTS (dst);
       sp && dp;
       sp = DECL_CHAIN (sp), dp = DECL_CHAIN (dp))
    {
      if (sp != dp)
	{
	  insert_decl_map (id, sp, as_value ? remap_as_value (sp, dp) : dp);
	  mapped = true;
	}
      if (src_num_artificial > 0)
	{
	  while (--src_num_artificial, src_num_artificial > 0)
	    sp = DECL_CHAIN (sp);
	}
      if (dst_num_artificial > 0)
	{
	  while (--dst_num_artificial, dst_num_artificial > 0)
	    dp = DECL_CHAIN (dp);
	}
    }
  return mapped;
}

/* Rewrite the condition of contract in place, so that references to SRC's
   parameters are updated to refer to DST's parameters. The postcondition
   result variable is left unchanged.

   When declarations are merged, we sometimes need to update contracts to
   refer to new parameters.

   If DUPLICATE_P is true, this is called by duplicate_decls to rewrite
   contracts in terms of a new set of parameters.  This also preserves the
   references to postcondition results, which are not replaced during
   merging.  */

static void
remap_contract (tree src, tree dst, tree contract, bool duplicate_p)
{
  copy_body_data id;
  hash_map<tree, tree> decl_map;

  memset (&id, 0, sizeof (id));
  id.src_fn = src;
  id.dst_fn = dst;
  id.src_cfun = DECL_STRUCT_FUNCTION (src);
  id.decl_map = &decl_map;

  /* If we're merging contracts, don't copy local variables.  */
  id.copy_decl = duplicate_p ? retain_decl : copy_decl_no_change;

  id.transform_call_graph_edges = CB_CGE_DUPLICATE;
  id.transform_new_cfg = false;
  id.transform_return_to_modify = false;
  id.transform_parameter = true;

  /* Make sure not to unshare trees behind the front-end's back
     since front-end specific mechanisms may rely on sharing.  */
  id.regimplify = false;
  id.do_not_unshare = true;
  id.do_not_fold = true;

  /* We're not inside any EH region.  */
  id.eh_lp_nr = 0;

  bool do_remap = false;

  /* Insert parameter remappings.  */
  gcc_checking_assert (TREE_CODE (src) == FUNCTION_DECL);
  gcc_checking_assert (TREE_CODE (dst) == FUNCTION_DECL);

  int src_num_artificial_args = num_artificial_parms_for (src);
  int dst_num_artificial_args = num_artificial_parms_for (dst);

  for (tree sp = DECL_ARGUMENTS (src), dp = DECL_ARGUMENTS (dst);
       sp || dp;
       sp = DECL_CHAIN (sp), dp = DECL_CHAIN (dp))
    {
      if (!sp)
	{
	  /* src params exhausted; remaining dst params are either __r
	     (postcondition result) or capture struct refs.  Map __r if
	     this is a postcondition.  */
	  if (dp && POSTCONDITION_P (contract))
	    {
	      gcc_assert (!duplicate_p);
	      if (tree result = POSTCONDITION_IDENTIFIER (contract))
		{
		  gcc_assert (DECL_P (result));
		  insert_decl_map (&id, result, remap_as_value (result, dp));
		  do_remap = true;
		}
	    }
	  break;
	}
      gcc_assert (sp && dp);

      if (sp == dp)
	continue;

      insert_decl_map (&id, sp, remap_as_value (sp, dp));
      do_remap = true;

      /* First artificial arg is *this. We want to remap that.  However, we
	 want to skip _in_charge param and __vtt_parm.  Do so now.  */
      if (src_num_artificial_args > 0)
	{
	  while (--src_num_artificial_args,src_num_artificial_args > 0)
	    sp = DECL_CHAIN (sp);
	}
      if (dst_num_artificial_args > 0)
	{
	  while (--dst_num_artificial_args,dst_num_artificial_args > 0)
	    dp = DECL_CHAIN (dp);
	}
    }

  if (!do_remap)
    return;

  /* A lambda's capture proxy is a local whose DECL_VALUE_EXPR reads the
     capture through the closure parameter.  Copying the proxy keeps that
     expression, still naming SRC's parameter; replace the proxy by its
     value expression, remapped like the rest.  */
  if (!duplicate_p && LAMBDA_FUNCTION_P (src))
    {
      hash_set<tree> proxies;
      auto collect = [] (tree *tp, int *, void *data) -> tree
	{
	  if (VAR_P (*tp) && is_capture_proxy (*tp))
	    static_cast<hash_set<tree> *> (data)->add (*tp);
	  return NULL_TREE;
	};
      cp_walk_tree_without_duplicates (&CONTRACT_CONDITION (contract),
				       collect, &proxies);
      for (tree proxy : proxies)
	{
	  tree ve = unshare_expr (DECL_VALUE_EXPR (proxy));
	  walk_tree (&ve, copy_tree_body_r, &id, NULL);
	  insert_decl_map (&id, proxy, ve);
	}
    }

  /* The generic walker does not enter a lambda, but the capture initializers
     of a lambda in the condition name parameters too; remap those
     separately.  (A cp_walk_tree of the whole condition would also remap
     the C++ trees the generic walk deliberately leaves alone, such as pack
     expansions.)  */
  auto_vec<tree> lambdas;
  auto collect_lambdas = [] (tree *tp, int *, void *data) -> tree
    {
      if (TREE_CODE (*tp) == LAMBDA_EXPR)
	static_cast<auto_vec<tree> *> (data)->safe_push (*tp);
      return NULL_TREE;
    };
  cp_walk_tree_without_duplicates (&CONTRACT_CONDITION (contract),
				   collect_lambdas, &lambdas);
  walk_tree (&CONTRACT_CONDITION (contract), copy_tree_body_r, &id, NULL);
  for (tree lambda : lambdas)
    for (tree cap = LAMBDA_EXPR_CAPTURE_LIST (lambda); cap;
	 cap = TREE_CHAIN (cap))
      walk_tree (&TREE_VALUE (cap), copy_tree_body_r, &id, NULL);
}

/* Returns a copy of SOURCE contracts where any references to SOURCE's
   PARM_DECLs have been rewritten to the corresponding PARM_DECL in DEST.  */

tree
copy_and_remap_contracts (tree dest, tree source,
			  contract_match_kind remap_kind)
{
  tree contracts = get_fn_contract_specifiers (source);
  if (!contracts)
    return NULL_TREE;

  /* An unparsed predicate has nothing to remap yet.  Copy the statements
     alone, so that parsing SOURCE's in place leaves these unparsed; whoever
     parses them copies again (cp_parser_replay_declaration_contracts).  */
  if (contract_any_deferred_p (contracts))
    {
      tree copy = make_tree_vec (TREE_VEC_LENGTH (contracts));
      for (int i = 0; i < TREE_VEC_LENGTH (contracts); ++i)
	TREE_VEC_ELT (copy, i) = copy_node (TREE_VEC_ELT (contracts, i));
      return copy;
    }

  /* POSITION indexes the contract within SOURCE's *full* contract list, in
     the same order used by compute_caller_semantic_tuple, so the wrapper's
     stored tuple can be read by position even though pre/post may be skipped
     below.  It is therefore taken for every contract, skipped or not.  */
  unsigned next_position = 0;
  auto_vec<tree> copies (TREE_VEC_LENGTH (contracts));
  for (tree contract : tree_vec_range (contracts))
    {
      const unsigned position = next_position++;

      if ((remap_kind == cmk_pre
	   && TREE_CODE (contract) == POSTCONDITION_STMT)
	  || (remap_kind == cmk_post
	      && TREE_CODE (contract) == PRECONDITION_STMT))
	continue;

      tree stmt = copy_node (contract);

      /* When copying contracts to a wrapper, set the evaluation semantic
	 appropriately.  For virtual function wrappers (P3097), the wrapper
	 evaluates contracts as callee-side checks, so keep the original
	 callee semantic.  For non-virtual wrappers, the wrapper evaluates
	 them caller-side, so use the caller semantic.  */
      if (DECL_IS_WRAPPER_FN_P (dest))
	{
	  bool virtual_wrapper = wrapper_for_virtual_dispatch_p (dest);
	  if (!virtual_wrapper)
	    {
	      unsigned char sem = get_wrapper_tuple_at (dest, position);
	      CONTRACT_EVALUATION_SEMANTIC (stmt)
		= build_int_cst (uint16_type_node, sem);

	      /* If the caller-side resolution for this contract is dynamic,
		 bake the descriptor + caller allowed-mask onto the wrapper's
		 copied contract so build_contract_check emits the dynamic
		 dispatch unchanged (it reads CONTRACT_DYNAMIC and, via
		 transform_semantic -> make_contract_query,
		 CONTRACT_ALLOWED_MASK).  */
	      tree desc = get_wrapper_dyn_at (dest, position);
	      if (desc)
		{
		  /* Reconstruct CONTRACT_DYNAMIC in the exact layout the
		     callee-side accessors read: TREE_PURPOSE = name
		     IDENTIFIER, TREE_VALUE = the packed INTEGER_CST.  */
		  CONTRACT_DYNAMIC (stmt)
		    = build_tree_list (TREE_PURPOSE (desc), TREE_VALUE (desc));

		  /* The dynamic transform clamps against the caller allowed
		     set, which includes IGNORE.  Fold IGNORE into the mask so
		     the emission (make_contract_query) sees the caller-side
		     set rather than the plain label mask.  */
		  tree m = CONTRACT_ALLOWED_MASK (stmt);
		  uint16_t mask
		    = (m ? (uint16_t) tree_to_uhwi (m)
		       : (uint16_t) CES_ALL_ALLOWED_WITH_EXTENSIONS);
		  mask |= (1 << CES_IGNORE);
		  CONTRACT_ALLOWED_MASK (stmt)
		    = build_int_cst (uint16_type_node, mask);
		}
	    }
	}

      /* If we have an erroneous postcondition identifier, we also mark the
	 condition as invalid so only need to check that.  */
      if (CONTRACT_CONDITION (stmt) != error_mark_node)
	remap_contract (source, dest, stmt, /*duplicate_p=*/true);

      if (TREE_CODE (stmt) == POSTCONDITION_STMT)
	{
	  /* If we have a postcondition return value placeholder, then
	     ensure the copied one has the correct context.  */
	  tree var = POSTCONDITION_IDENTIFIER (stmt);
	  if (var && var != error_mark_node)
	    DECL_CONTEXT (var) = dest;

	  /* Deep-copy postcondition captures (P3098) so the wrapper has
	     its own VAR_DECLs with independent DECL_INITIAL.  The inline
	     capture init machinery destructively clears DECL_INITIAL after
	     use, so sharing VAR_DECLs between the wrapper and the original
	     function would cause whichever runs second to see NULL inits.
	     We also remap references to old capture vars in the condition
	     to point to the new copies.  */
	  tree caps = POSTCONDITION_CAPTURES (stmt);
	  if (caps)
	    {
	      copy_body_data cap_id;
	      hash_map<tree, tree> cap_decl_map;
	      memset (&cap_id, 0, sizeof (cap_id));
	      cap_id.src_fn = source;
	      cap_id.dst_fn = dest;
	      cap_id.decl_map = &cap_decl_map;
	      cap_id.copy_decl = retain_decl;
	      cap_id.transform_call_graph_edges = CB_CGE_DUPLICATE;
	      cap_id.do_not_unshare = true;
	      cap_id.do_not_fold = true;

	      /* Map SOURCE's parameters (and *this) to DEST's so a recovered
		 capture initializer that references the interface parameters
		 refers to the wrapper's parameters.  */
	      map_corresponding_parms (&cap_id, source, dest,
				       /*as_value=*/false);
	      /* A parameter that is not remapped -- SOURCE and DEST may share
		 them -- stays itself, and is not copied with the initializers
		 below.  */
	      for (tree fn : { source, dest })
		for (tree p = DECL_ARGUMENTS (fn); p; p = DECL_CHAIN (p))
		  if (!cap_decl_map.get (p))
		    insert_decl_map (&cap_id, p, p);

	      tree new_caps = NULL_TREE;
	      tree *last_cap = &new_caps;
	      for (tree cap = caps; cap; cap = TREE_CHAIN (cap))
		{
		  tree orig_var = TREE_VALUE (cap);
		  tree new_var = copy_node (orig_var);
		  cxx_dup_lang_specific_decl (new_var);
		  DECL_CONTEXT (new_var) = dest;
		  /* Structurally copy + remap the initializer (parameters,
		     *this), so that DEST's capture reads DEST's parameters --
		     a capture written on a declaration that is not the
		     definition must read the definition's -- and the shared
		     tree is not mutated.  Recover it first if SOURCE's inline
		     emission already cleared it destructively (see
		     postcondition_capture_inits); otherwise the wrapper's
		     capture is left uninitialized.  */
		  tree init = DECL_INITIAL (new_var);
		  if (!init && postcondition_capture_inits)
		    if (tree *saved
			  = postcondition_capture_inits->get (orig_var))
		      init = *saved;
		  if (init)
		    {
		      copy_body_data init_id = cap_id;
		      init_id.copy_decl = copy_decl_no_change;
		      init_id.do_not_unshare = false;
		      walk_tree (&init, copy_tree_body_r, &init_id, NULL);
		      DECL_INITIAL (new_var) = init;
		    }
		  insert_decl_map (&cap_id, orig_var, new_var);
		  tree node = tree_cons (TREE_PURPOSE (cap), new_var,
					 NULL_TREE);
		  *last_cap = node;
		  last_cap = &TREE_CHAIN (node);
		}
	      POSTCONDITION_CAPTURES (stmt) = new_caps;

	      /* Remap capture var references in the condition.  */
	      walk_tree (&CONTRACT_CONDITION (stmt), copy_tree_body_r,
			 &cap_id, NULL);
	    }
	}

      if (CONTRACT_COMMENT (stmt) != error_mark_node)
	CONTRACT_COMMENT (stmt) = copy_node (CONTRACT_COMMENT (stmt));

      copies.quick_push (stmt);
    }

  if (copies.is_empty ())
    return NULL_TREE;

  tree contracts_copy = make_tree_vec (copies.length ());
  for (unsigned ix = 0; ix < copies.length (); ix++)
    TREE_VEC_ELT (contracts_copy, ix) = copies[ix];

  return contracts_copy;
}

/* Set the (maybe) parsed contract specifiers CONTRACTS for DECL.
   CONTRACTS is either  NULL_TREE or a TREE_VEC of contract statements.  */

void
set_fn_contract_specifiers (tree decl, tree contracts)
{
  if (!decl || error_operand_p (decl))
    return;

  gcc_checking_assert (!contracts || TREE_CODE (contracts) == TREE_VEC);

  bool existed = false;
  contract_decl& rd
    = hash_map_safe_get_or_insert<hm_ggc> (contract_decl_map, decl, &existed);
  if (!existed)
    {
      /* This is the first time we encountered this decl, save the location
	 for error messages.  This will ensure all error messages refer to the
	 contracts used for the function.  */
      location_t decl_loc = DECL_SOURCE_LOCATION (decl);
      location_t cont_end = decl_loc;
      if (contracts)
	cont_end = get_contract_end_loc (contracts);
      rd.note_loc = make_location (decl_loc, decl_loc, cont_end);
    }
  rd.contract_specifiers = contracts;
}

/* Update the entry for DECL in the map of contract specifiers with the
  contracts in CONTRACTS.  */

void
update_fn_contract_specifiers (tree decl, tree contracts)
{
  if (!decl || error_operand_p (decl))
    return;

  bool existed = false;
  contract_decl& rd
    = hash_map_safe_get_or_insert<hm_ggc> (contract_decl_map, decl, &existed);
  gcc_checking_assert (existed);

  /* We should only get here when we parse deferred contracts.  */
  gcc_checking_assert (!contract_any_deferred_p (contracts));

  rd.contract_specifiers = contracts;
}

/* When a decl is about to be removed, then we need to release its content and
   then take it out of the map.  */

void
remove_decl_with_fn_contracts_specifiers (tree decl)
{
  if (contract_decl *p = hash_map_safe_get (contract_decl_map, decl))
    {
      p->contract_specifiers = NULL_TREE;
      contract_decl_map->remove (decl);
    }
}

/* If this function has contract specifiers, then remove them, but leave the
   function registered.  */

void
remove_fn_contract_specifiers (tree decl)
{
  if (contract_decl *p = hash_map_safe_get (contract_decl_map, decl))
    {
      p->contract_specifiers = NULL_TREE;
    }
}

/* Get the contract specifier list for this DECL if there is one.  */

tree
get_fn_contract_specifiers (tree decl)
{
  if (contract_decl *p = hash_map_safe_get (contract_decl_map, decl))
    return p->contract_specifiers;
  return NULL_TREE;
}

/* A redeclaration match that could not be performed where redeclarations are
   merged, because one side's predicate had not been parsed yet.

   Every function contract is now token-cached at its declarator and replayed
   once the function's parameters can be put back in scope, so by the time
   duplicate_decls runs the two predicates being compared may still be
   DEFERRED_PARSE.  Comparing them there would compare two unparsed nodes and
   silently accept any mismatch.  The comparison is recorded instead and run
   from flush_deferred_contract_matches, once both sides have been parsed.

   The contract vectors are kept rather than the declarations: duplicate_decls
   merges newdecl into olddecl and the former does not survive, while the
   vectors are the operands match_contract_specifiers actually reads, and the
   late parse updates their conditions in place.  The locations are taken here
   for the same reason -- they must describe where the declarations were
   written, not wherever the parser has reached when the match finally runs.  */

struct GTY(()) deferred_contract_match
{
  tree old_contracts;
  tree new_contracts;
  /* The declaration that survives the merge; it supplies the parameters the
     redeclaration's predicate is parsed against.  */
  tree fndecl;
  /* The redeclaration's parameter names, kept because the declaration itself
     does not survive to be asked.  */
  tree new_parm_names;
  location_t old_loc;
  location_t new_loc;
  /* True when the entry is not a comparison but the check that a
     redeclaration does not ADD contracts to a function first declared
     without them.  OLD_CONTRACTS is null for those, there being none, and
     OLD_LOC is the first declaration rather than a contract on it.  */
  bool adds_p;
};

static GTY(()) vec<deferred_contract_match, va_gc> *deferred_contract_matches;

/* The parameter names of DECL as a TREE_LIST, so that they can be compared
   after DECL has been reclaimed.  */

static tree
contract_parm_name_list (tree decl)
{
  tree names = NULL_TREE;
  for (tree parm = DECL_ARGUMENTS (decl); parm; parm = DECL_CHAIN (parm))
    names = tree_cons (NULL_TREE, DECL_NAME (parm), names);
  return nreverse (names);
}

/* Whether DECL has exactly as many parameters as NAMES lists.  The names
   themselves need not agree: the predicate is parsed with DECL's parameters
   bound under the redeclaration's names, so a renamed parameter resolves to
   the right one positionally.  Only the count has to line up.  */

static bool
contract_parm_count_matches_p (tree decl, tree names)
{
  tree parm = DECL_ARGUMENTS (decl);
  for (; parm && names; parm = DECL_CHAIN (parm), names = TREE_CHAIN (names))
    ;
  return !parm && !names;
}

/* Run every recorded match that can now be run.  Called after each late
   parse.  LATE_PARSE is supplied by the parser, which owns the token caches.

   An entry becomes runnable once the surviving declaration's own predicate
   has been parsed.  The redeclaration's predicate usually has not been and
   never will be: duplicate_decls drops its contract_decl_map entry and frees
   the declaration, so nothing walks it.  It is parsed here instead, against
   the surviving declaration's parameters, bound under the names the
   redeclaration itself used -- the two may spell them differently, and a
   definition in between will have replaced the names with its own.  Only a
   differing parameter COUNT defeats that, and two declarations of one
   function cannot differ there.  */

void
flush_deferred_contract_matches (late_contract_parse_fn late_parse)
{
  static bool flushing = false;

  if (!deferred_contract_matches || flushing)
    return;

  temp_override<bool> guard (flushing, true);

  unsigned ix = 0;
  while (ix < deferred_contract_matches->length ())
    {
      /* By value: the entry is removed while it is still in use below.  */
      deferred_contract_match m = (*deferred_contract_matches)[ix];

      if (m.adds_p)
	{
	  if (contract_any_deferred_p (m.new_contracts))
	    {
	      if (!late_parse
		  || !m.fndecl
		  || !contract_parm_count_matches_p (m.fndecl,
						     m.new_parm_names))
		{
		  deferred_contract_matches->ordered_remove (ix);
		  continue;
		}
	      late_parse (m.fndecl, m.new_contracts, m.new_parm_names);
	      if (contract_any_deferred_p (m.new_contracts))
		{
		  deferred_contract_matches->ordered_remove (ix);
		  continue;
		}
	    }

	  /* A predicate that did not parse has already been reported at the
	     point it was written.  It never became a contract, so it did not
	     add one either.  */
	  if (contract_all_valid_p (m.new_contracts))
	    {
	      auto_diagnostic_group d;
	      error_at (m.new_loc, "declaration adds contracts to %q#D",
			m.fndecl);
	      inform (m.old_loc, "first declared here");
	    }

	  /* Rejected either way, so they are not this function's contracts.
	     Dropping them is what lets a THIRD declaration still be compared
	     with the first, which has none: while they stayed recorded, the
	     next redeclaration was compared against the very contracts that
	     had just been refused, and so was either accepted in silence or
	     reported as a mismatch rather than as another addition.  */
	  remove_fn_contract_specifiers (m.fndecl);
	  deferred_contract_matches->ordered_remove (ix);
	  continue;
	}

      /* Still waiting on the surviving declaration.  */
      if (contract_any_deferred_p (m.old_contracts))
	{
	  ++ix;
	  continue;
	}

      if (contract_any_deferred_p (m.new_contracts))
	{
	  if (!late_parse
	      || !m.fndecl
	      || !contract_parm_count_matches_p (m.fndecl, m.new_parm_names))
	    {
	      deferred_contract_matches->ordered_remove (ix);
	      continue;
	    }

	  late_parse (m.fndecl, m.new_contracts, m.new_parm_names);

	  /* If it still did not parse, there is nothing to compare.  */
	  if (contract_any_deferred_p (m.new_contracts))
	    {
	      deferred_contract_matches->ordered_remove (ix);
	      continue;
	    }
	}

      match_contract_specifiers (m.old_loc, m.old_contracts,
				 m.new_loc, m.new_contracts);
      deferred_contract_matches->ordered_remove (ix);
    }
}

/* A subroutine of duplicate_decls. Diagnose issues in the redeclaration of
   guarded functions.  */

void
check_redecl_contract (tree newdecl, tree olddecl)
{
  if (!flag_contracts)
    return;

  if (TREE_CODE (newdecl) == TEMPLATE_DECL)
    newdecl = DECL_TEMPLATE_RESULT (newdecl);
  if (TREE_CODE (olddecl) == TEMPLATE_DECL)
    olddecl = DECL_TEMPLATE_RESULT (olddecl);

  /* An implicit instantiation is not a declaration anyone wrote: an explicit
     specialization that register_specialization merges with one is the
     first declaration of that specialization, and its contracts are its
     own.  */
  if (DECL_IMPLICIT_INSTANTIATION (olddecl)
      && DECL_TEMPLATE_SPECIALIZATION (newdecl))
    {
      /* OLDDECL is the node that survives the merge.  It has exactly
	 NEWDECL's contracts -- none, if NEWDECL has none: an explicit
	 specialization does not inherit the ones the implicit instantiation
	 was given from the template -- onto its own parameters (the tokens,
	 while deferred), so later redeclarations compare with them.  */
      tree nc = get_fn_contract_specifiers (newdecl);
      if (!nc)
	remove_fn_contract_specifiers (olddecl);
      else if (contract_any_deferred_p (nc))
	{
	  /* The tokens are replayed against OLDDECL's parameters, which
	     still carry the names the template gave them.  OLDDECL is the
	     specialization from here on, so give them the names it wrote:
	     `template<> int g<double> (double a) pre (a > 1);' must find
	     `a'.  A definition replaces the parameters with its own anyway;
	     a declaration keeps OLDDECL's, so this is the only point at
	     which the specialization's names can reach them.  */
	  for (tree op = DECL_ARGUMENTS (olddecl),
		 np = DECL_ARGUMENTS (newdecl);
	       op && np; op = DECL_CHAIN (op), np = DECL_CHAIN (np))
	    DECL_NAME (op) = DECL_NAME (np);
	  set_fn_contract_specifiers (olddecl, nc);
	}
      else
	set_fn_contract_specifiers (olddecl,
				    copy_and_remap_contracts (olddecl,
							      newdecl));
      return;
    }

  tree new_contracts = get_fn_contract_specifiers (newdecl);
  tree old_contracts = get_fn_contract_specifiers (olddecl);

  if (!old_contracts && !new_contracts)
    return;

  /* We should always be comparing with the 'first' declaration which should
   have been recorded already (if it has contract specifiers).  However
   if the new decl is trying to add contracts, that is an error and we do
   not want to create a map entry yet.  */
  contract_decl *rdp = hash_map_safe_get (contract_decl_map, olddecl);
  gcc_checking_assert(rdp || !old_contracts);

  location_t new_loc = DECL_SOURCE_LOCATION (newdecl);
  /* An explicit specialization is a function of its own, so this holds for
     its redeclarations too.  */
  if (new_contracts && !old_contracts)
    {
      /* If a re-declaration has contracts, they must be the same as those
       that appear on the first declaration seen (they cannot be added).  */
      location_t cont_end = get_contract_end_loc (new_contracts);
      cont_end = make_location (new_loc, new_loc, cont_end);

      /* Whether there is anything to complain about is not yet known when
	 the predicate has not been parsed: one that fails to parse never
	 becomes a contract, and is reported where it was written, so
	 "declaration adds contracts" on top of it is noise.  Record the
	 check and run it from flush_deferred_contract_matches, which is
	 also the only point at which the contracts can be dropped again --
	 update_contract_arguments copies them onto the surviving
	 declaration after this runs, whatever is decided here.  */
      if (contract_any_deferred_p (new_contracts))
	{
	  deferred_contract_match m
	    = { NULL_TREE, new_contracts, olddecl,
		contract_parm_name_list (newdecl),
		DECL_SOURCE_LOCATION (olddecl), cont_end, /*adds_p=*/true };
	  vec_safe_push (deferred_contract_matches, m);
	  return;
	}

      auto_diagnostic_group d;
      error_at (cont_end, "declaration adds contracts to %q#D", olddecl);
      inform (DECL_SOURCE_LOCATION (olddecl), "first declared here");
      return;
    }

  if (old_contracts && !new_contracts)
    /* We allow re-declarations to omit contracts declared on the initial decl.
       In fact, this is required if the conditions contain lambdas.  Check if
       all the parameters are correctly const qualified. */
    check_postconditions_in_redecl (olddecl, newdecl);
  /* A friend redeclaration is not special here.  It was, back when a
     friend's were the only deferred contracts and there was nowhere to
     record a comparison that could not yet be made; the queue below is that
     place, and a friend belongs in it like any other redeclaration.  Taking
     the friend out of the queue is what left one accepted however it was
     spelled -- undeclared names and contradictory conditions alike --
     because the queue is also the only thing that parses it.  */
  else if (contract_any_deferred_p (old_contracts)
	   || contract_any_deferred_p (new_contracts))
    {
      /* One side has not been parsed yet, so the two cannot be compared
	 here.  Record the comparison and run it from
	 flush_deferred_contract_matches once both have been.  Every function
	 contract is deferred to its late parse, so doing nothing here would
	 skip redeclaration matching entirely and accept any mismatch in
	 silence.  */
      location_t cont_end = get_contract_end_loc (new_contracts);
      cont_end = make_location (new_loc, new_loc, cont_end);
      deferred_contract_match m
	= { old_contracts, new_contracts, olddecl,
	    contract_parm_name_list (newdecl), rdp->note_loc, cont_end,
	    /*adds_p=*/false };
      vec_safe_push (deferred_contract_matches, m);
    }
  else
    {
      gcc_checking_assert (old_contracts);
      location_t cont_end = get_contract_end_loc (new_contracts);
      cont_end = make_location (new_loc, new_loc, cont_end);
      /* We have two sets - they should match or we issue a diagnostic.  */
      match_contract_specifiers (rdp->note_loc, old_contracts,
				 cont_end, new_contracts);
    }

  return;
}

/* DECL declares an explicit specialization of a member template of a class
   template specialization:

     template<> template<class Q> int A<long>::f (long a, Q) pre (a > 3);

   check_explicit_specialization folds it into RESULT, the pattern of the
   member template A<long> instantiated, rather than having duplicate_decls
   merge the two, so do here what duplicate_decls does for the contracts.
   REDECL_P if RESULT already is that specialization and DECL redeclares it;
   DEFINING_P if DECL is a definition, whose parameters replace RESULT's.
   grokfndecl gives RESULT DECL's contracts afterwards if it has none.  */

void
member_template_spec_contracts (tree decl, tree result, bool redecl_p,
				bool defining_p)
{
  if (!redecl_p)
    {
      /* The first declaration of the specialization.  The contracts the
	 instantiated member was given from the template are not the
	 specialization's.  Its own predicates are replayed against RESULT's
	 parameters, which still carry the names the template gave them, so
	 give them the names DECL wrote, as check_redecl_contract does for an
	 implicit instantiation; a definition replaces them anyway.  */
      remove_fn_contract_specifiers (result);
      if (!defining_p)
	for (tree op = DECL_ARGUMENTS (result), np = DECL_ARGUMENTS (decl);
	     op && np; op = DECL_CHAIN (op), np = DECL_CHAIN (np))
	  DECL_NAME (op) = DECL_NAME (np);
      return;
    }

  /* A redeclaration: compare its contracts with the specialization's.  */
  check_redecl_contract (decl, result);

  if (!defining_p)
    return;

  /* A definition's parameters are about to replace RESULT's.  Its own
     contracts are what its body is checked against, as
     update_contract_arguments has it; without any, RESULT's are rewritten
     in terms of the definition's parameters.  */
  tree contracts = get_fn_contract_specifiers (decl);
  if (contracts)
    set_fn_contract_specifiers (result, contracts);
  else if ((contracts = get_fn_contract_specifiers (result))
	   && !contract_any_deferred_p (contracts))
    {
      contracts = copy_and_remap_contracts (decl, result);
      /* The copies' own decls belong to RESULT, which survives, not to
	 DECL.  */
      for (tree c : tree_vec_range (contracts))
	if (POSTCONDITION_P (c))
	  {
	    tree var = POSTCONDITION_IDENTIFIER (c);
	    if (var && var != error_mark_node)
	      DECL_CONTEXT (var) = result;
	    for (tree cap = POSTCONDITION_CAPTURES (c); cap;
		 cap = TREE_CHAIN (cap))
	      DECL_CONTEXT (TREE_VALUE (cap)) = result;
	  }
      set_fn_contract_specifiers (result, contracts);
    }
}

/* Update the contracts of DEST to match the argument names from contracts
  of SRC. When we merge two declarations in duplicate_decls, we preserve the
  arguments from the new declaration, if the new declaration is a
  definition. We need to update the contracts accordingly.  */

void
update_contract_arguments (tree srcdecl, tree destdecl)
{
  tree src_contracts = get_fn_contract_specifiers (srcdecl);
  tree dest_contracts = get_fn_contract_specifiers (destdecl);

  if (!src_contracts && !dest_contracts)
    return;

  /* Check if src even has contracts. It is possible that a redeclaration
    does not have contracts. Is this is the case, first apply contracts
    to src.  */
  if (!src_contracts)
    {
      if (contract_any_deferred_p (dest_contracts))
	{
	  set_fn_contract_specifiers (srcdecl, dest_contracts);
	  /* Nothing more to do here.  */
	  return;
	}
      else
	set_fn_contract_specifiers
	  (srcdecl, copy_and_remap_contracts (srcdecl, destdecl));
    }

  /* For deferred contracts, we currently copy the tokens from the redeclaration
    onto the decl that will be preserved. This is not ideal because the
    redeclaration may have erroneous contracts.
    For non deferred contracts we currently do copy and remap, which is doing
    more than we need.

    The copy is unconditional on purpose, and it is tempting to think it
    should not be: the first declaration's contracts are the function's, so
    a later declaration's look like they exist only to be compared.  An
    out-of-line DEFINITION also arrives here as SRCDECL, though, and its own
    token cache is what its body has to be checked against -- withhold the
    copy and it is checked against the in-class text instead.  */
  if (contract_any_deferred_p (src_contracts))
    set_fn_contract_specifiers (destdecl, src_contracts);
  else
    {
      /* Temporarily rename the arguments to get the right mapping.  */
      tree tmp_arguments = DECL_ARGUMENTS (destdecl);
      DECL_ARGUMENTS (destdecl) = DECL_ARGUMENTS (srcdecl);
      set_fn_contract_specifiers (destdecl,
				  copy_and_remap_contracts (destdecl, srcdecl));
      DECL_ARGUMENTS (destdecl) = tmp_arguments;
    }
}

/* Compute the ordered caller-side semantic tuple for the contracts of the
   callee FNDECL, resolved for the call site (CALLER_LOC, CALLER_FNDECL).
   Returns a TREE_LIST whose Nth TREE_VALUE is an INTEGER_CST giving the
   resolved caller-side semantic for the Nth contract in FNDECL's full
   contract list (DECL_ORIGIN order), matching copy_and_remap_contracts.
   D4405: if ANALYSES is non-null, the Nth element pushed to it is what the
   Nth contract's caller-side resolution gives the analysis evaluation at
   the call (caller_resolution::analysis).  */

static tree
compute_caller_semantic_tuple (tree fndecl, location_t caller_loc,
			       tree caller_fndecl,
			       vec<contract_evaluation_semantic> *analyses)
{
  tree tuple = NULL_TREE, *last = &tuple;
  tree specs = get_fn_contract_specifiers (DECL_ORIGIN (fndecl));
  if (!specs)
    return tuple;

  for (tree contract : tree_vec_range (specs))
    {
      caller_resolution r
	= resolve_caller_semantic (contract, DECL_ORIGIN (fndecl),
				   caller_loc, caller_fndecl);
      if (analyses)
	analyses->safe_push (r.analysis);
      /* When the caller-side resolution is dynamic, carry the descriptor in
	 the tuple element's TREE_PURPOSE so it can key the wrapper and be
	 baked into the wrapper's copied contract.  The layout matches the
	 callee-side CONTRACT_DYNAMIC cache: TREE_PURPOSE = selector name
	 IDENTIFIER, TREE_VALUE = INTEGER_CST packing (configured << 8
	 | linkage << 1 | provideweak).  TREE_PURPOSE == NULL_TREE means not
	 dynamic.  */
      tree desc = NULL_TREE;
      if (r.dyn_name)
	{
	  unsigned HOST_WIDE_INT packed
	    = ((unsigned HOST_WIDE_INT) r.dyn_configured << 8)
	      | ((unsigned HOST_WIDE_INT) r.dyn_linkage << 1)
	      | (r.dyn_provideweak ? 1 : 0);
	  desc = build_tree_list (get_identifier (r.dyn_name),
				  build_int_cst (uint16_type_node, packed));
	}
      tree node
	= build_tree_list (desc, build_int_cst (uint16_type_node, r.semantic));
      *last = node;
      last = &TREE_CHAIN (node);
    }
  return tuple;
}

/* Possibly replace call with a call to a wrapper function which
   will do the contracts check required around a CALL to FNDECL.  */

tree
maybe_contract_wrap_call (tree fndecl, tree call, bool is_virtual_dispatch)
{
  /* We can be called from build_cxx_call without a known callee.  */
  if (!fndecl)
    return call;

  if (error_operand_p (fndecl) || !call || call == error_mark_node)
    return error_mark_node;

  /* P4405 proving hook: a call of an inheriting constructor.  */
  if (flag_contracts && !processing_template_decl)
    diagnose_inheriting_ctor_call (fndecl, call);

  if (!handle_caller_contracts_p (fndecl))
    return call;

  /* For virtual dispatch with P3097, always wrap -- the wrapper uses
     callee-side semantics for the interface contracts.  For non-virtual
     calls (including qualified calls to virtual functions), resolve the
     caller-side semantic tuple for this call site and check whether any
     entry is active.  */
  bool is_virtual = (is_virtual_dispatch
		     && flag_contracts_p3097
		     && DECL_IOBJ_MEMBER_FUNCTION_P (fndecl)
		     && DECL_VIRTUAL_P (fndecl));

  /* Virtual wrappers use callee semantics and share a single wrapper per
     callee, keyed by the empty (NULL_TREE) sentinel tuple.  Non-virtual
     wrappers are keyed by the resolved caller-semantic tuple.  */
  tree tuple = NULL_TREE;
  bool any_active = is_virtual;
  /* P4405 proving hook: the markers of deferred caller-side evaluations,
     evaluated before the call.  Only for a precondition.  Most calls are
     preceded by them (a COMPOUND_EXPR).  A constructor or destructor call,
     or one returning a class, has to stay a call for its users
     (build_cplus_new, AGGR_INIT_EXPR, cleanups), so it carries them
     instead (CARRIER):
     - a constructor or destructor call whose object exists, in its object
       argument, which is evaluated before the call as well (and which a
       constexpr constructor call folded to a constant keeps,
       contract_object_proof_markers);
     - a call that initializes a temporary -- one returning a class, or a
       constructor call whose object is a placeholder -- is made the
       initialization of that temporary here, with the markers ahead of the
       call in its TARGET_EXPR_INITIAL, as the comma operator puts its left
       operand (cp_build_compound_expr).  The placeholder is not the object:
       a precondition that names `this' is given the temporary's slot
       (TEMP, built early for it; GCC-926), which an initialization that
       elides the temporary replaces in the markers too
       (cp_gimplify_init_expr).
     PLACEHOLDER_CTOR: a constructor call whose object is still a
     placeholder (a prvalue, `T ()'), which its callers make the
     initialization of a temporary (build_cplus_new) whose slot replaces
     the placeholder.  */
  tree temp = NULL_TREE;
  tree this_call = NULL_TREE;
  bool placeholder_ctor = (TREE_CODE (call) == CALL_EXPR
			   && DECL_CONSTRUCTOR_P (fndecl)
			   && call_expr_nargs (call) > 0
			   && is_dummy_object (CALL_EXPR_ARG (call, 0)));
  enum { MARK_BEFORE, MARK_OBJECT, MARK_TEMPORARY } carrier = MARK_BEFORE;
  if (DECL_CONSTRUCTOR_P (fndecl) || DECL_DESTRUCTOR_P (fndecl))
    carrier = placeholder_ctor ? MARK_TEMPORARY : MARK_OBJECT;
  else if (MAYBE_CLASS_TYPE_P (TREE_TYPE (call)))
    carrier = MARK_TEMPORARY;
  tree proof_markers = NULL_TREE;
  bool marker_call_p = (contract_proof_deferral_ok_p (current_function_decl)
			&& !processing_template_decl
			&& TREE_CODE (call) == CALL_EXPR
			&& (carrier != MARK_OBJECT
			    || call_expr_nargs (call) > 0));
  /* D4405: the analysis evaluations at this call -- each precondition's,
     and what a caller-side prove or diagnose gives any contract
     (call_analysis_semantic) -- are diagnosed here, at the call, unless
     proven -- with the call's arguments available to the proof, and for a
     specialization whose contracts are not yet substituted, a substituted
     copy of its predicate (contract_for_caller_side_proof).  They have no
     run-time effect: a caller-side prove or diagnose leaves the caller-side
     runtime evaluation ignore, so it neither causes a wrapper to be built
     nor is diagnosed again inside one built for another contract.  A
     virtual call (P3097) is a call of the function it names, whose
     preconditions it analyzes.  A call in an unevaluated operand evaluates
     nothing, and one manifestly constant-evaluated has no analysis
     evaluation (contract_constant_evaluation_p).  A call in a contract
     predicate is evaluated only if a check of the predicate is, which is
     not known here: it is diagnosed where such a check is emitted
     (diagnose_checked_predicate_calls).  The
     inheriting constructor's call of the constructor it inherits was
     diagnosed at each call of the inheriting one
     (diagnose_inheriting_ctor_call).  */
  prepare_call_analysis (fndecl);
  auto_vec<contract_evaluation_semantic> analyses;
  if (!is_virtual)
    tuple = compute_caller_semantic_tuple (fndecl, input_location,
					   current_function_decl, &analyses);
  tree specs = get_fn_contract_specifiers (DECL_ORIGIN (fndecl));
  tree inh_self
    = (current_function_decl
       ? inherited_ctor_for_proving (current_function_decl) : NULL_TREE);
  bool skip = (cp_unevaluated_operand
	       || processing_contract_condition
	       || contract_constant_evaluation_p (fndecl)
	       || (inh_self && inh_self == DECL_ORIGIN (fndecl)));
  /* A temporary's destructor call is analyzed only if the temporary is
     not elided, which is known later (contract_resolve_pending_analysis):
     its object argument carries what that needs.  */
  if (!skip
      && carrier == MARK_OBJECT
      && DECL_DESTRUCTOR_P (fndecl)
      && TREE_CODE (call) == CALL_EXPR
      && call_expr_nargs (call) > 0
      && !processing_template_decl
      && contract_temporary_cleanup_p ()
      && contract_call_analyzed_p (fndecl, analyses))
    {
      tree arg = CALL_EXPR_ARG (call, 0);
      tree pending
	= contract_defer_call_analysis (fndecl,
					cp_expr_loc_or_input_loc (call),
					analyses);
      CALL_EXPR_ARG (call, 0)
	= build2 (COMPOUND_EXPR, TREE_TYPE (arg), pending, arg);
    }
  else
    proof_markers = contract_analyze_call (fndecl, call,
					   cp_expr_loc_or_input_loc (call),
					   analyses, skip, marker_call_p,
					   placeholder_ctor, &temp,
					   &this_call);

  if (!is_virtual)
    {
      /* P3100: a caller-side assume is an optimizer assumption, made in the
	 wrapper, which inlining brings into the caller.  */
      unsigned position = 0;
      for (tree t = tuple; t && !any_active; t = TREE_CHAIN (t), position++)
	if (contract_lowered_as_assumption_p (TREE_VEC_ELT (specs, position),
					      tree_to_uhwi (TREE_VALUE (t))))
	  any_active = true;
      for (tree t = tuple; t; t = TREE_CHAIN (t))
	/* A dynamic descriptor (TREE_PURPOSE non-null) forces the wrapper to
	   be emitted even when the compile-time default is ignore/assume: the
	   selector may return a checking semantic at run time.  This mirrors
	   the callee-side contract_active_p, which forces active whenever
	   CONTRACT_DYNAMIC is present.  Gating on an actual descriptor (not on
	   a label merely having a compute_semantic facet) preserves the
	   caller-side opt-in invariant.  */
	if (TREE_PURPOSE (t)
	    || !contract_semantic_emits_no_check
		  (tree_to_uhwi (TREE_VALUE (t))))
	  {
	    any_active = true;
	    break;
	  }
    }

  /* P4405 proving hook: the markers come before the call or the wrapper
     call, or in the call's object argument, which the wrapper call copies,
     or in the initializer of the temporary the call initializes (CARRIER).
     A constructor call for a placeholder object initializes its temporary
     here whenever the call is not left as it was: with a caller-side
     wrapper, which is called in place of the constructor (GCC-921: the
     wrapper is not a constructor, so build_cplus_new would not replace the
     placeholder by the temporary's slot), or with proof markers.  */
  if (proof_markers && carrier == MARK_OBJECT)
    {
      tree arg = CALL_EXPR_ARG (call, 0);
      CALL_EXPR_ARG (call, 0)
	= build2 (COMPOUND_EXPR, TREE_TYPE (arg), proof_markers, arg);
      proof_markers = NULL_TREE;
    }
  auto with_proof_markers = [=] (tree c) {
    if (placeholder_ctor || (proof_markers && carrier == MARK_TEMPORARY))
      {
	/* Built as the callers would (build_cplus_new), for a constructor
	   from the constructor call, so that its AGGR_INIT_EXPR replaces the
	   placeholder.  Should that fail (an incomplete or abstract class),
	   the callers' build_cplus_new diagnoses it.  */
	tree type = (placeholder_ctor
		     ? TREE_TYPE (TREE_TYPE (CALL_EXPR_ARG (call, 0)))
		     : TREE_TYPE (c));
	tree t = (temp && TREE_CODE (temp) == TARGET_EXPR ? temp
		  : build_cplus_new (type, placeholder_ctor ? call : c,
				     tf_none));
	if (TREE_CODE (t) != TARGET_EXPR)
	  return c;
	tree init = TARGET_EXPR_INITIAL (t);
	if (placeholder_ctor && c != call)
	  {
	    gcc_checking_assert (TREE_CODE (init) == AGGR_INIT_EXPR
				 && AGGR_INIT_VIA_CTOR_P (init));
	    AGGR_INIT_EXPR_FN (init) = CALL_EXPR_FN (c);
	  }
	if (proof_markers)
	  TARGET_EXPR_INITIAL (t)
	    = build2 (COMPOUND_EXPR, TREE_TYPE (init), proof_markers, init);
	return t;
      }
    if (!proof_markers)
      return c;
    tree r = build2 (COMPOUND_EXPR, TREE_TYPE (c), proof_markers, c);
    suppress_warning (r, OPT_Wunused_value);
    return r;
  };
  if (proof_markers && !any_active)
    return with_proof_markers (call);

  if (!any_active)
    return call;

  /* Build the declaration of the wrapper, if we need to.  */
  tree wrapdecl = get_or_create_contract_wrapper_function (fndecl, tuple);

  unsigned nargs = call_expr_nargs (call);
  vec<tree, va_gc> *argwrap;
  vec_alloc (argwrap, nargs);

  tree arg;
  call_expr_arg_iterator iter;
  FOR_EACH_CALL_EXPR_ARG (arg, iter, call)
    argwrap->quick_push (arg);

  tree wrapcall = build_call_expr_loc_vec (DECL_SOURCE_LOCATION (wrapdecl),
					   wrapdecl, argwrap);

  /* P4405 proving hook.  */
  if (proof_markers || placeholder_ctor)
    return with_proof_markers (wrapcall);
  return wrapcall;
}

/* Define a single wrapper function WRAPDECL that wraps callee FNDECL.
   This generates code for client-side contract check wrappers and the
   noexcept wrapper around the contract violation handler.  Returns true
   if the wrapper is (now or already) defined.  */

static bool
define_one_contract_wrapper_func (tree fndecl, tree wrapdecl)
{
  /* If we already built this function on a previous pass, then do nothing.  */
  if (DECL_INITIAL (wrapdecl) && DECL_INITIAL (wrapdecl) != error_mark_node)
    return true;

  gcc_checking_assert (!DECL_HAS_CONTRACTS_P (wrapdecl));

  /* This wrapper is about to copy FNDECL's contracts and emit checks from
     them, and it is the one reader of those contracts that does not need
     FNDECL to have a definition: a pure virtual has none, and neither does a
     declaration-only template reached by a P3595 caller-side check.  For
     either, nothing has substituted them yet -- that normally happens when the
     definition is instantiated -- so the copy below would be the pattern's own
     dependent tree.  The same holds for a template whose definition exists
     but is not instantiated before this (a constant-evaluated call defines
     the wrapper on demand) or not in this translation unit at all (an
     explicit instantiation declaration), hence FOR_WRAPPER.  A no-op for
     everything else.

     DECL_ORIGIN, to match what the contracts are actually read from below: for
     a wrapper around a CDTOR clone that is the original, not the clone.  */
  maybe_instantiate_contracts (DECL_ORIGIN (fndecl), /*for_wrapper=*/true);

  bool is_virtual = wrapper_for_virtual_dispatch_p (wrapdecl);

  /* For virtual functions, always include postconditions -- the wrapper uses
     callee-side semantics (P3097).  For non-virtual, check whether any
     postcondition is active caller-side according to this wrapper's stored
     caller-semantic tuple.  Positions are counted over the full contract
     list, matching compute_caller_semantic_tuple /
     copy_and_remap_contracts.  */
  bool check_post = is_virtual;
  if (!check_post)
    {
      tree specs = get_fn_contract_specifiers (DECL_ORIGIN (fndecl));
      unsigned nspecs = specs ? (unsigned) TREE_VEC_LENGTH (specs) : 0;
      for (unsigned position = 0; position < nspecs; position++)
	{
	  tree contract = TREE_VEC_ELT (specs, position);
	  if (!POSTCONDITION_P (contract))
	    continue;
	  /* P3100: an assumption, as in maybe_contract_wrap_call.  */
	  if (contract_lowered_as_assumption_p
		(contract, get_wrapper_tuple_at (wrapdecl, position)))
	    {
	      check_post = true;
	      break;
	    }
	  /* A dynamic postcondition (descriptor present) must be checked even
	     when its compile-time default is ignore/assume -- the selector may
	     return a checking semantic at run time.  Mirrors the activeness
	     forcing in maybe_contract_wrap_call.  */
	  if (get_wrapper_dyn_at (wrapdecl, position)
	      || !contract_semantic_emits_no_check
		    (get_wrapper_tuple_at (wrapdecl, position)))
	    {
	      check_post = true;
	      break;
	    }
	}
    }

  /* For wrappers on CDTORs we need to refer to the original contracts,
     when the wrapper is around a clone.  */
  set_fn_contract_specifiers (wrapdecl,
		    copy_and_remap_contracts (wrapdecl, DECL_ORIGIN (fndecl),
					     check_post ? cmk_all : cmk_pre));

  /* P4298: unlike build_contract_condition_function's outlined .pre/.post
     functions (which contain nothing but contract-check code), WRAPDECL
     also calls through to FNDECL's real implementation -- code with no
     relationship to contract-evaluation semantics at all.  Marking
     WRAPDECL noexcept purely because its checked contracts are all
     nonthrowing would incorrectly also force termination on a legitimate
     exception thrown by that implementation.  WRAPDECL's type already
     inherits FNDECL's exception specification verbatim (in
     build_contract_wrapper_function), so it is noexcept here if and only
     if FNDECL itself is -- no additional marking based on contract
     semantics is applied.  */

  /* The body returns the callee's call; for a noreturn callee that return
     is never reached, but it is still a return statement in a function
     declared noreturn, so build the body without the flag.  */
  bool noreturn_p = TREE_THIS_VOLATILE (wrapdecl);
  TREE_THIS_VOLATILE (wrapdecl) = false;

  start_preparsed_function (wrapdecl, /*DECL_ATTRIBUTES*/NULL_TREE,
			    SF_DEFAULT | SF_PRE_PARSED);
  tree body = begin_function_body ();
  tree compound_stmt = begin_compound_stmt (BCS_FN_BODY);

  vec<tree, va_gc> *args = build_arg_list (wrapdecl);

  /* If this is a virtual member function, dispatch through the vtable so
     that the final overrider's callee-side contracts are checked (P3097
     two-source model).  Otherwise, call the function directly.  */
  tree fn = fndecl;
  if (is_virtual)
    {
      tree *class_ptr = args->begin ();
      gcc_checking_assert (class_ptr);

      tree binfo = lookup_base (TREE_TYPE (TREE_TYPE (*class_ptr)),
				DECL_CONTEXT (fndecl),
				ba_any, NULL, tf_warning_or_error);
      gcc_checking_assert (binfo && binfo != error_mark_node);

      *class_ptr = build_base_path (PLUS_EXPR, *class_ptr, binfo, 1,
				    tf_warning_or_error);
      if (TREE_SIDE_EFFECTS (*class_ptr))
	*class_ptr = save_expr (*class_ptr);
      tree t = build_pointer_type (TREE_TYPE (fndecl));
      fn = build_vfn_ref (*class_ptr, DECL_VINDEX (fndecl));
      TREE_TYPE (fn) = t;
    }

  /* Pass a C-variadic callee's variadic arguments on unchanged; see
     build_contract_wrapper_function.  */
  if (stdarg_p (TREE_TYPE (fndecl)))
    vec_safe_push (args, build_call_expr (builtin_decl_explicit
					    (BUILT_IN_VA_ARG_PACK), 0));

  /* A reference result is returned as the lvalue it designates, as
     build_cxx_call returns it: finish_return_stmt expects no expression of
     reference type (GCC-678).  */
  tree call
    = convert_from_reference (build_thunk_like_call (fn, args->length (),
						     args->address ()));

  finish_return_stmt (call);

  finish_compound_stmt (compound_stmt);
  finish_function_body (body);
  tree fn_done = finish_function (/*inline_p=*/false);
  TREE_THIS_VOLATILE (wrapdecl) = noreturn_p;
  expand_or_defer_fn (fn_done);
  return true;
}

/* On-demand definition of a contract wrapper's body.  Contract wrappers are
   normally defined at end of TU (emit_contract_wrapper_func); the constexpr
   evaluator calls this to materialize a constexpr wrapper's body the first time
   a constant evaluation needs it -- otherwise a constexpr virtual (or
   caller-side) function carrying a contract would be "used before its
   definition" in a constant expression.  Returns true if WRAPDECL is (now or
   already) defined.  */

bool
maybe_define_contract_wrapper (tree wrapdecl)
{
  if (!wrapdecl || !DECL_CONTRACT_WRAPPER (wrapdecl))
    return false;
  if (DECL_INITIAL (wrapdecl) && DECL_INITIAL (wrapdecl) != error_mark_node)
    return true;
  tree fndecl = get_orig_func_for_wrapper (wrapdecl);
  if (!fndecl || fndecl == error_mark_node)
    return false;
  /* The evaluation can be in the middle of parsing a function, a class or a
     template; define the wrapper from the top level, as synthesize_method
     does, so that none of that context is lost or leaks into it.  */
  bool push_to_top = maybe_push_to_top_level (wrapdecl);
  bool defined = define_one_contract_wrapper_func (fndecl, wrapdecl);
  maybe_pop_from_top_level (push_to_top);
  return defined;
}

/* Map traversal callback: FNDECL maps to a TREE_LIST of (tuple, wrapdecl)
   pairs (see decl_wrapper_fn).  Define each wrapper that wraps FNDECL.  */

bool
define_contract_wrapper_func (const tree& fndecl, const tree& wrappers, void*)
{
  for (tree p = wrappers; p; p = TREE_CHAIN (p))
    define_one_contract_wrapper_func (fndecl, TREE_VALUE (p));
  return true;
}

/* Return the total number of (tuple, wrapper) pairs recorded across all
   callees in decl_wrapper_fn.  A single callee may have several wrappers.  */

static size_t
count_wrapper_pairs (void)
{
  if (!decl_wrapper_fn)
    return 0;
  size_t n = 0;
  for (auto it = decl_wrapper_fn->begin ();
       it != decl_wrapper_fn->end (); ++it)
    for (tree p = (*it).second; p; p = TREE_CHAIN (p))
      n++;
  return n;
}

/* If any wrapper functions have been declared, emit their definition.
   This might be called multiple times, as we instantiate functions. When
   the processing here adds more wrappers, then flag to the caller that
   possible additional instantiations should be considered.
   Once instantiations are complete, this will be called with done == true.  */

bool
emit_contract_wrapper_func (bool done)
{
  if (!decl_wrapper_fn || decl_wrapper_fn->is_empty ())
    return false;
  size_t start_pairs = count_wrapper_pairs ();
  decl_wrapper_fn->traverse<void *, define_contract_wrapper_func>(NULL);
  bool more = count_wrapper_pairs () > start_pairs;
  if (done)
    decl_wrapper_fn->empty ();
  gcc_checking_assert (!done || !more);
  return more;
}

/* Mark most of a contract as being invalid.  */

tree
invalidate_contract (tree contract)
{
  if (TREE_CODE (contract) == POSTCONDITION_STMT
      && POSTCONDITION_IDENTIFIER (contract))
    POSTCONDITION_IDENTIFIER (contract) = error_mark_node;
  CONTRACT_CONDITION (contract) = error_mark_node;
  CONTRACT_COMMENT (contract) = error_mark_node;
  return contract;
}

/* Returns an invented parameter declaration of the form 'TYPE ID' for the
   purpose of parsing the postcondition.

   We use a PARM_DECL instead of a VAR_DECL so that tsubst forces a lookup
   in local specializations when we instantiate these things later.  */

/* [basic.scope.contract]/2: a result name that is not name-independent
   may not potentially conflict with a declaration whose target scope is the
   function parameter scope of the function, or, for a lambda, the nearest
   enclosing lambda scope.  DECL is the result name, not yet pushed into the
   contract scope.  This holds whatever the evaluation semantic and whether
   or not the declaration is a definition, so it is checked here, where the
   result name is made.  */

static void
check_result_name_conflict (tree decl)
{
  /* A result-name-introducer _ is name-independent ([basic.scope.scope]);
     name_independent_decl_p does not know PARM_DECLs.  */
  if (id_equal (DECL_NAME (decl), "_"))
    return;
  tree seen = lookup_name (DECL_NAME (decl));
  if (!seen)
    return;

  /* The scope the contract's function declares its parameters in is the
     nearest one outside the contract scope.  */
  cp_binding_level *scope = current_binding_level;
  while (scope && scope->kind == sk_contract)
    scope = scope->level_chain;

  bool parm_p = false;
  if (TREE_CODE (seen) == PARM_DECL && !DECL_ARTIFICIAL (seen) && scope)
    {
      if (scope->kind == sk_function_parms)
	{
	  for (cxx_binding *b = IDENTIFIER_BINDING (DECL_NAME (decl)); b;
	       b = b->previous)
	    if (b->value == seen)
	      {
		parm_p = b->scope == scope;
		break;
	      }
	}
      else
	/* A lambda's contracts are parsed once its captures are in scope,
	   inside its body; its parameters are then those of the function
	   being defined.  */
	parm_p = (current_function_decl
		  && LAMBDA_FUNCTION_P (current_function_decl)
		  && DECL_CONTEXT (seen) == current_function_decl);
    }
  /* An init-capture of the lambda whose contract this is.  A simple capture
     declares nothing, so its proxy does not count.  */
  bool init_capture_p
    = (!parm_p && current_function_decl
       && LAMBDA_FUNCTION_P (current_function_decl)
       && is_capture_proxy (seen) && !is_normal_capture_proxy (seen)
       && DECL_CONTEXT (seen) == current_function_decl);
  if (!parm_p && !init_capture_p)
    return;

  auto_diagnostic_group d;
  error_at (DECL_SOURCE_LOCATION (decl),
	    parm_p
	    ? G_("contract postcondition result name shadows a function "
		 "parameter")
	    : G_("contract postcondition result name shadows a lambda "
		 "capture"));
  inform (DECL_SOURCE_LOCATION (seen),
	  parm_p ? G_("parameter declared here") : G_("capture declared here"));
}

tree
make_postcondition_variable (cp_expr id, tree type)
{
  if (id == error_mark_node)
    return id;
  gcc_checking_assert (scope_chain && scope_chain->bindings
		       && scope_chain->bindings->kind == sk_contract);

  tree decl = build_lang_decl (PARM_DECL, id, type);
  DECL_ARTIFICIAL (decl) = true;
  DECL_SOURCE_LOCATION (decl) = id.get_location ();
  check_result_name_conflict (decl);
  return pushdecl (decl);
}

/* As above, except that the type is unknown.  */

tree
make_postcondition_variable (cp_expr id)
{
  return make_postcondition_variable (id, make_auto ());
}

/* Check that the TYPE is valid for a named postcondition variable on
   function decl FNDECL. Emit a diagnostic if it is not.  Returns TRUE if
   the result is OK and false otherwise.  */

bool
check_postcondition_result (tree fndecl, tree type, location_t loc)
{
  /* Do not be confused by targetm.cxx.cdtor_return_this ();
     conceptually, cdtors have no return value.  */
  if (VOID_TYPE_P (type)
      || DECL_CONSTRUCTOR_P (fndecl)
      || DECL_DESTRUCTOR_P (fndecl))
    {
      error_at (loc,
		DECL_CONSTRUCTOR_P (fndecl)
		? G_("constructor does not return a value to test")
		: DECL_DESTRUCTOR_P (fndecl)
		? G_("destructor does not return a value to test")
		: G_("function does not return a value to test"));
      return false;
    }

  return true;
}

/* Callback for contract_condition_uses_pack_p.  */

static tree
find_pack_use_r (tree *tp, int *walk_subtrees, void *)
{
  tree t = *tp;

  if (PACK_EXPANSION_P (t)
      || TREE_CODE (t) == NONTYPE_ARGUMENT_PACK
      || TREE_CODE (t) == TYPE_ARGUMENT_PACK
      || (DECL_P (t) && DECL_PACK_P (t))
      || (TREE_CODE (t) == TEMPLATE_TYPE_PARM
	  && TEMPLATE_TYPE_PARAMETER_PACK (t)))
    {
      *walk_subtrees = 0;
      return t;
    }

  /* A declaration's type is not walked by cp_walk_tree, but a reference to
     a pack parameter carries the pack-ness there.  */
  if (DECL_P (t) && TREE_TYPE (t) && PACK_EXPANSION_P (TREE_TYPE (t)))
    {
      *walk_subtrees = 0;
      return t;
    }

  return NULL_TREE;
}

/* True if CONDITION mentions a parameter pack in any form.  Distinct from
   uses_parameter_packs, which reports only packs that are still
   *unexpanded*: a fold-expression expands its pack, so that predicate says
   "no packs" for exactly the conditions that matter here.  */

static bool
contract_condition_uses_pack_p (tree condition)
{
  return cp_walk_tree_without_duplicates (&condition, find_pack_use_r, NULL)
	 != NULL_TREE;
}

/* Instantiate each postcondition with the return type to finalize the
   contract specifiers on a function decl FNDECL.  */

void
rebuild_postconditions (tree fndecl)
{
  if (!fndecl || fndecl == error_mark_node || processing_template_decl)
    return;

  tree type = TREE_TYPE (TREE_TYPE (fndecl));

  /* If the return type is undeduced, defer until later.  */
  if (type_uses_auto (type))
    return;

  tree contract_spec = get_fn_contract_specifiers (fndecl);
  if (!contract_spec)
    return;

  for (tree contract : tree_vec_range (contract_spec))
    {
      if (TREE_CODE (contract) != POSTCONDITION_STMT)
	continue;
      tree condition = CONTRACT_CONDITION (contract);
      if (!condition || condition == error_mark_node)
	continue;

      /* If any conditions are deferred, they're all deferred.  Note that
	 we don't have to instantiate postconditions in that case because
	 the type is available through the declaration.  */
      if (TREE_CODE (condition) == DEFERRED_PARSE)
	return;

      tree oldvar = POSTCONDITION_IDENTIFIER (contract);
      if (!oldvar)
	continue;

      gcc_checking_assert (!DECL_CONTEXT (oldvar)
			   || DECL_CONTEXT (oldvar) == fndecl);
      DECL_CONTEXT (oldvar) = fndecl;

      /* Check the postcondition variable.  */
      location_t loc = DECL_SOURCE_LOCATION (oldvar);
      if (!check_postcondition_result (fndecl, type, loc))
	{
	  invalidate_contract (contract);
	  continue;
	}

      /* A concrete late-parsed result variable still needs validation, but
	 not rebuilding.  Rebuild only one whose type was undeduced.  */
      if (!type_uses_auto (TREE_TYPE (oldvar)))
	continue;

      /* A condition mentioning a parameter pack cannot go through the
	 substitution below.  It is deliberately handed the empty argument
	 vector, relying only on the local identity mappings installed here
	 -- but TMPL_ARGS_DEPTH reports depth 1 for a zero-length TREE_VEC
	 (only NULL_TREE gives 0), so tsubst_pack_expansion believes there
	 is an argument level to read and indexes out of the empty vector.

	 Nothing needs doing here in any case: tsubst_contract rebuilds the
	 result variable against the real return type and substitutes the
	 condition with real arguments at instantiation.  A pack can only
	 appear inside a template, so that path always runs.  */
      if (contract_condition_uses_pack_p (condition))
	continue;

      /* "Instantiate" the result variable using the known type.  */
      tree newvar = copy_node (oldvar);
      TREE_TYPE (newvar) = type;

      /* Make parameters, result, and captures available for substitution.  */
      local_specialization_stack stack (lss_copy);
      for (tree t = DECL_ARGUMENTS (fndecl); t != NULL_TREE; t = TREE_CHAIN (t))
	register_local_identity (t);
      register_local_specialization (newvar, oldvar);

      /* Register captures as identity mappings so tsubst_expr handles
	 capture references correctly (especially pack captures, which
	 remain unexpanded during this substitution).  */
      tree caps = POSTCONDITION_CAPTURES (contract);
      if (caps && TREE_CODE (TREE_VALUE (caps)) == VAR_DECL)
	for (tree cap = caps; cap; cap = TREE_CHAIN (cap))
	  register_local_identity (TREE_VALUE (cap));

      begin_scope (sk_contract, fndecl);
      bool old_pc = processing_postcondition_predicate;
      processing_postcondition_predicate = true;

      condition = tsubst_expr (condition, make_tree_vec (0),
			       tf_warning_or_error, fndecl);

      /* Update the contract condition and result.  */
      POSTCONDITION_IDENTIFIER (contract) = newvar;
      CONTRACT_CONDITION (contract) = finish_contract_condition (condition);
      /* [dcl.contract.func]/7 was already applied to this predicate when it
	 was late-parsed (update_late_contract); re-substituting the result
	 variable does not change which parameters it odr-uses.  */
      processing_postcondition_predicate = old_pc;
      gcc_checking_assert (scope_chain && scope_chain->bindings
			   && scope_chain->bindings->kind == sk_contract);
      pop_bindings_and_leave_scope ();
    }
}

/* Extract a STRING_CST from a constant-evaluated const char* result.
   The result may be NOP_EXPR(ADDR_EXPR(STRING_CST)) or similar.  */

static tree
extract_string_from_const_char_ptr (tree result)
{
  if (!result || result == error_mark_node)
    return NULL_TREE;
  STRIP_NOPS (result);
  if (TREE_CODE (result) == ADDR_EXPR)
    {
      tree operand = TREE_OPERAND (result, 0);
      if (TREE_CODE (operand) == STRING_CST)
	return operand;
      if (VAR_P (operand) && DECL_INITIAL (operand)
	  && TREE_CODE (DECL_INITIAL (operand)) == STRING_CST)
	return DECL_INITIAL (operand);
    }
  if (TREE_CODE (result) == STRING_CST)
    return result;
  return NULL_TREE;
}

/* Apply a label's string-transforming facet (compute_comment or
   compute_message) to the current value.  Returns the transformed
   STRING_CST wrapped in build_string_literal, or the original on failure.  */

static tree
apply_label_string_facet (tree label, const char *facet_name,
			  tree current_val, location_t loc)
{
  /* In a template the facet waits for instantiation, where
     reresolve_contract_label_facets applies it once to the substituted
     contract: evaluating it here does not work in a template body, and
     applying it here too would apply it twice.  */
  if (processing_template_decl)
    return current_val;

  if (!label || label == error_mark_node
      || !TREE_TYPE (label)
      || type_dependent_expression_p (label))
    return current_val;

  tree label_type = TREE_TYPE (label);
  if (!CLASS_TYPE_P (label_type))
    return current_val;

  tree fn_id = get_identifier (facet_name);
  tree fn = lookup_member (label_type, fn_id,
			   /*protect=*/0, /*want_type=*/false, tf_none);
  if (!fn || fn == error_mark_node
      || !label_facet_accessible_p (label_type, fn))
    return current_val;

  /* The facet takes a const char* to the current string: a pointer into a
     real string literal, so that the facet may do pointer arithmetic on it
     in a constant expression.  */
  tree arg;
  if (!current_val)
    arg = build_zero_cst (const_string_type_node);
  else if (TREE_CODE (current_val) == STRING_CST)
    arg = build_string_literal (TREE_STRING_LENGTH (current_val),
				TREE_STRING_POINTER (current_val));
  else
    arg = current_val;
  vec<tree, va_gc> *args = NULL;
  vec_safe_push (args, arg);
  tree call = build_new_method_call (label_facet_object (label), fn, &args,
				     NULL_TREE, LOOKUP_NORMAL,
				     NULL, tf_none);
  if (!call || call == error_mark_node)
    return current_val;
  /* The result need only be convertible to const char*.  */
  call = perform_implicit_conversion (const_string_type_node, call, tf_none);
  if (call == error_mark_node)
    return current_val;

  tree result = constant_facet_value (call, label_type, facet_name, loc);
  tree str = NULL_TREE;
  if (const char *p = (result == error_mark_node ? NULL
		       : c_getstr (result)))
    str = build_string (strlen (p) + 1, p);
  else
    str = extract_string_from_const_char_ptr (result);
  if (str)
    {
      if (!current_val || TREE_CODE (current_val) == STRING_CST)
	return build_string (TREE_STRING_LENGTH (str),
			     TREE_STRING_POINTER (str));
      return build_string_literal (TREE_STRING_LENGTH (str),
				   TREE_STRING_POINTER (str));
    }
  return current_val;
}

/* Make a string of the contract condition, if it is available.  */

static tree
build_comment (cp_expr condition, char *str)
{
  /* -fcontract-comment-from-expression: use the pretty-printed form even
     when the source text is available.  */
  if (str && flag_contract_comment_from_expression)
    {
      free (str);
      str = NULL;
    }
  if (!str)
    {
      const char *str = expr_to_string (condition);
      return build_string_literal (strlen (str) + 1, str);
    }

  tree t = build_string_literal (strlen (str) + 1, str);
  free (str);
  return t;
}

/* Build a contract statement.  */

static tree lookup_std_contracts_type (tree);
static tree build_local_violation_trampoline (tree, tree, tree *);
static tree build_query_trampoline (tree);
static tree contracts_tu_local_named_var (location_t, const char *, tree);

/* The string half of finish_contract_message: MESSAGE as a bare STRING_CST,
   or NULL_TREE if it is absent or invalid.  In a template a message that is
   not a string literal is returned as written (type-checked unless it is
   dependent): extracting it needs constant evaluation, which waits for
   tsubst_contract, as finish_static_assert's does.  So does a message whose
   CONDITION is still deferred.  */

tree
finish_contract_message_string (tree message, tree condition, location_t loc)
{
  if (message)
    message = tree_strip_any_location_wrapper (message);
  if (!message || TREE_CODE (message) == STRING_CST
      || message == error_mark_node)
    return message == error_mark_node ? NULL_TREE : message;

  if (processing_template_decl && type_dependent_expression_p (message))
    return message;
  cexpr_str cstr (message);
  if (!cstr.type_check (loc))
    return NULL_TREE;
  if (processing_template_decl || TREE_CODE (condition) == DEFERRED_PARSE)
    return message;

  const char *msg;
  int len;
  if (!cstr.extract (loc, msg, len))
    return NULL_TREE;
  char *buf = XNEWVEC (char, len + 1);
  memcpy (buf, msg, len);
  buf[len] = '\0';
  message = build_string (len + 1, buf);
  XDELETEVEC (buf);
  return message;
}

/* Normalize a just-parsed contract diagnostic message (P3099) into a bare
   STRING_CST (or NULL_TREE), then apply the compute_message facet (P3400),
   storing the result in CONTRACT_MESSAGE (CONTRACT) and returning it.

   String literals arrive from the parser inside a location wrapper, and
   non-literal expressions need cexpr_str validation + constant-folding to
   extract the string.  Extraction is only possible once CONDITION is no longer
   a DEFERRED_PARSE; for a deferred (late-parsed member-function) contract the
   caller invokes this again from the late-parse path once the condition is
   available, so the message ends up a bare STRING_CST for every contract --
   the invariant every CONTRACT_MESSAGE consumer relies on.  */

tree
finish_contract_message (tree contract, tree message, tree condition,
			 location_t loc)
{
  CONTRACT_MESSAGE (contract)
    = finish_contract_message_string (message, condition, loc);

  /* A deferred contract's message is deferred with its condition, and this
     is called again from the late parse once both are available; applying
     the facet now would apply it to no message at all.  */
  if (TREE_CODE (condition) == DEFERRED_PARSE)
    return CONTRACT_MESSAGE (contract);

  /* Apply compute_message facet (P3400) if present.  */
  CONTRACT_MESSAGE (contract)
    = apply_label_string_facet (CONTRACT_LABEL (contract), "compute_message",
				CONTRACT_MESSAGE (contract), loc);
  return CONTRACT_MESSAGE (contract);
}

/* After a template-dependent LABEL has been substituted to a concrete
   value, discard the label-derived state CONTRACT inherited from the
   pattern and recompute what only the label can supply.

   Two things are stale at that point.  The lazily-cached slots -- groups
   and the two evaluation semantics and the dynamic descriptor -- may
   already have been filled against the *pattern*, because caller-side
   resolution runs at the call site, before the definition is
   instantiated.  ensure_contract_groups in particular writes
   error_mark_node when it sees a still-dependent label, and copy_node
   then carries that onto the instantiation, where it makes every later
   group lookup early-out -- so a templated contract silently lost its
   group_names.  And the string facets are only ever applied at parse
   time, where the label was dependent and they were skipped outright, so
   compute_comment and compute_message never ran for a templated contract
   at all.  */

void
reresolve_contract_label_facets (tree contract, tree label, location_t loc)
{
  if (!label || label == error_mark_node
      || type_dependent_expression_p (label))
    return;

  CONTRACT_GROUPS (contract) = NULL_TREE;
  CONTRACT_EVALUATION_SEMANTIC (contract) = NULL_TREE;
  CONTRACT_CONSTEXPR_EVALUATION_SEMANTIC (contract) = NULL_TREE;
  CONTRACT_DYNAMIC (contract) = NULL_TREE;

  CONTRACT_COMMENT (contract)
    = apply_label_string_facet (label, "compute_comment",
				CONTRACT_COMMENT (contract), loc);
  CONTRACT_MESSAGE (contract)
    = apply_label_string_facet (label, "compute_message",
				CONTRACT_MESSAGE (contract), loc);
}

/* Build the argument list a facet named FACET_NAME is probed with.  Returns
   NULL when the facet takes a shape this does not know how to probe.  */

static vec<tree, va_gc> *
build_facet_probe_args (const char *facet_name)
{
  vec<tree, va_gc> *args = NULL;

  if (!strcmp (facet_name, "handle_contract_violation"))
    {
      tree cv
	= lookup_std_contracts_type (get_identifier ("contract_violation"));
      if (!cv || cv == error_mark_node)
	return NULL;
      tree ref = cp_build_indirect_ref
	(input_location,
	 build_zero_cst (build_pointer_type
			 (cp_build_qualified_type (cv, TYPE_QUAL_CONST))),
	 RO_UNARY_STAR, tf_none);
      if (!ref || ref == error_mark_node)
	return NULL;
      vec_safe_push (args, ref);
      return args;
    }

  if (!strcmp (facet_name, "query"))
    {
      tree cvp = build_pointer_type
	(cp_build_qualified_type (void_type_node, TYPE_QUAL_CONST));
      vec_safe_push (args, build_zero_cst (cvp));
      vec_safe_push (args, build_zero_cst (size_type_node));
      return args;
    }

  if (!strcmp (facet_name, "compute_comment")
      || !strcmp (facet_name, "compute_message"))
    {
      vec_safe_push (args, build_zero_cst (const_string_type_node));
      return args;
    }

  if (!strcmp (facet_name, "compute_semantic"))
    {
      tree arg = build_semantic_facet_arg (2);
      if (!arg)
	return NULL;
      vec_safe_push (args, arg);
      return args;
    }

  return NULL;
}

/* True when calling FACET_NAME on OBJ with ARGS is viable.  ACCESS says
   whether access control applies.  */

static bool
facet_call_viable_p (tree obj, tree fn, vec<tree, va_gc> *args,
		     deferring_kind access)
{
  deferring_access_check_sentinel sentry (access);
  vec<tree, va_gc> *a = make_tree_vector_copy (args);
  tree call = build_new_method_call (obj, fn, &a, NULL_TREE, LOOKUP_NORMAL,
				     NULL, tf_none);
  return call && call != error_mark_node;
}

/* Warn when LABEL_TYPE has a member named FACET_NAME that almost provides a
   facet but does not.

   Only two near misses are reported, alone or together, and each is
   recognized by relaxing it and seeing whether that makes the call viable:
   an inaccessible member, one that is not const, and one that is both.
   Anything else stays silent.  That matters: P3400 points out that users
   legitimately give a label private helpers sharing a facet's name --
   tag-dispatch overloads, say -- and warning on a name match alone would fire
   on every one of them.  Relaxing access and constness will not make such a
   helper's signature fit, so it never reaches a warning.  */

static void
warn_near_miss_facet (location_t loc, tree label, tree label_type,
		      const char *facet_name)
{
  if (!warn_contract_invalid_label_facet)
    return;

  tree fn = lookup_member (label_type, get_identifier (facet_name),
			   /*protect=*/0, /*want_type=*/false, tf_none);
  if (!fn || fn == error_mark_node)
    return;

  vec<tree, va_gc> *args = build_facet_probe_args (facet_name);
  if (!args)
    return;

  /* The question detection asked: a const object, access enforced.  If that
     is viable the facet is present and there is nothing to report.  */
  if (facet_call_viable_p (label, fn, args, dk_no_deferred))
    return;

  tree decl = BASELINK_P (fn) ? BASELINK_FUNCTIONS (fn) : fn;
  if (TREE_CODE (decl) == OVERLOAD)
    decl = OVL_FIRST (decl);

  /* Relax access only.  */
  if (!label_facet_accessible_p (label_type, fn)
      && facet_call_viable_p (label, fn, args, dk_no_check))
    {
      if (warning_at (loc, OPT_Wcontract_invalid_label_facet,
		      "%qT does not provide the %qs facet because that member "
		      "is inaccessible", label_type, facet_name)
	  && DECL_P (decl))
	inform (DECL_SOURCE_LOCATION (decl), "declared here");
      return;
    }

  /* Relax const only.  A facet is always invoked on a constexpr, therefore
     const, control object, so a non-const member is not a facet.  */
  tree nonconst_obj = cp_build_indirect_ref
    (loc, build_zero_cst (build_pointer_type (label_type)), RO_UNARY_STAR,
     tf_none);
  if (!nonconst_obj || nonconst_obj == error_mark_node)
    return;
  if (facet_call_viable_p (nonconst_obj, fn, args, dk_no_deferred))
    {
      if (warning_at (loc, OPT_Wcontract_invalid_label_facet,
		      "%qT does not provide the %qs facet because that member "
		      "is not %<const%>; a facet member must be %<const%> or "
		      "%<static%>", label_type, facet_name)
	  && DECL_P (decl))
	inform (DECL_SOURCE_LOCATION (decl), "declared here");
      return;
    }

  /* Relax both.  */
  if (!label_facet_accessible_p (label_type, fn)
      && facet_call_viable_p (nonconst_obj, fn, args, dk_no_check))
    {
      if (warning_at (loc, OPT_Wcontract_invalid_label_facet,
		      "%qT does not provide the %qs facet because that member "
		      "is inaccessible and not %<const%>; a facet member must "
		      "be %<const%> or %<static%>", label_type, facet_name)
	  && DECL_P (decl))
	inform (DECL_SOURCE_LOCATION (decl), "declared here");
    }
}

/* Whether a member of type TYPE has the shape of the data-member facet
   FACET_NAME, its access and constness aside: for allowed_semantics,
   `evaluation_semantic_set ({m})' is valid; for group_names, `m[0]' is and
   converts to `const char *' ([support.contract.control.allowed],
   [support.contract.control.ident]).  */

static bool
data_facet_shape_p (location_t loc, tree type, const char *facet_name)
{
  tree obj = cp_build_indirect_ref
    (loc, build_zero_cst (build_pointer_type (cv_unqualified (type))),
     RO_UNARY_STAR, tf_none);
  if (!obj || obj == error_mark_node)
    return false;

  if (!strcmp (facet_name, "allowed_semantics"))
    {
      tree set_type
	= lookup_std_contracts_type (get_identifier ("evaluation_semantic_set"));
      if (!set_type || set_type == error_mark_node)
	return false;
      tree init = build_constructor_single (init_list_type_node, NULL_TREE,
					    obj);
      tree set = build_functional_cast (loc, set_type,
					build_tree_list (NULL_TREE, init),
					tf_none);
      return set && set != error_mark_node;
    }

  tree elt = grok_array_decl (loc, obj, size_zero_node, NULL, tf_none);
  if (!elt || elt == error_mark_node)
    return false;
  tree conv = perform_implicit_conversion (const_string_type_node, elt,
					   tf_none);
  return conv && conv != error_mark_node;
}

/* Warn when LABEL_TYPE has a data member named FACET_NAME, one of the
   data-member facets, that has the facet's shape but is not one because it
   is inaccessible, not const, or both.  A same-named member of another shape
   stays silent, as in warn_near_miss_facet.  */

static void
warn_near_miss_data_facet (location_t loc, tree label_type,
			   const char *facet_name)
{
  if (!warn_contract_invalid_label_facet)
    return;

  tree member = lookup_member (label_type, get_identifier (facet_name),
			       /*protect=*/0, /*want_type=*/false, tf_none);
  if (!member || member == error_mark_node || BASELINK_P (member))
    return;
  if (!VAR_P (member) && TREE_CODE (member) != FIELD_DECL)
    return;

  bool inaccessible = !label_facet_accessible_p (label_type, member);
  bool nonconst = !CP_TYPE_CONST_P (TREE_TYPE (member));
  if (!inaccessible && !nonconst)
    return;
  if (!data_facet_shape_p (loc, TREE_TYPE (member), facet_name))
    return;

  const char *why = (inaccessible && nonconst
		     ? G_("%qT does not provide the %qs facet because that "
			  "member is inaccessible and not %<const%>")
		     : inaccessible
		     ? G_("%qT does not provide the %qs facet because that "
			  "member is inaccessible")
		     : G_("%qT does not provide the %qs facet because that "
			  "member is not %<const%>"));
  if (warning_at (loc, OPT_Wcontract_invalid_label_facet, why, label_type,
		  facet_name))
    inform (DECL_SOURCE_LOCATION (member), "declared here");
}

/* Map from label type -> local violation handler trampoline FUNCTION_DECL.
   Generated at parse time, looked up during gimplification.  */
static GTY(()) hash_map<tree, tree, pch_traits> *local_violation_trampoline_map;

/* Label types already checked for near-miss facets, so that a label used on a
   hundred contracts does not warn a hundred times.  */
static GTY(()) hash_set<tree> *near_miss_checked_types;

/* Map from label type -> the user's handle_contract_violation FUNCTION_DECL
   that the corresponding trampoline calls.  Recorded so that the rethrow
   analysis (contract_local_handler_always_rethrows_p) examines exactly the
   function the trampoline will call, rather than repeating the member lookup
   and risking a different overload resolution.  */
static GTY(()) hash_map<tree, tree, pch_traits> *local_violation_handler_fn_map;

/* Map from label type -> query trampoline FUNCTION_DECL.
   Generated at parse time, looked up during gimplification.  */
static GTY(()) hash_map<tree, tree, pch_traits> *query_trampoline_map;

/* Validate LABEL (P3400 assertion-control label) for CONTRACT and compute
   its derived facets: assertion_control_object structural validity, the
   local-violation/queryable-label trampolines, and the allowed-semantics
   restriction mask.  A no-op if LABEL is absent, invalid, or still
   type-dependent.

   Called from grok_contract for a label available at parse time -- which
   includes an in-class-defined ("deferred") member function's label: only
   its predicate is deferred, never the label -- and again from
   tsubst_contract once a template-dependent label has been substituted to a
   concrete value at instantiation.

   Takes no tsubst_flags_t and reports unconditionally.  That is safe only
   because the instantiation-time path cannot be reached from a SFINAE
   context: tsubst_contract_specifiers has a single caller,
   regenerate_decl_from_template, which passes tf_warning_or_error.  Give
   this a complain parameter if that ever stops being true.  */

void
resolve_contract_label (tree contract, tree label, location_t loc)
{
  if (!label || label == error_mark_node
      || type_dependent_expression_p (label))
    return;

  /* Check access immediately rather than deferring it, for the duration of
     facet detection.  Every probe below is a SFINAE-style question -- "does
     this type participate in this facet?" -- asked with tf_none, and with
     immediate checking an inaccessible member makes the probe fail quietly,
     which is the answer the concepts give.  Deferred checking instead queued
     the failure and reported it later as an error against the contract's
     predicate, turning "this label has no local handler" into a diagnostic.  */
  deferring_access_check_sentinel access_sentinel (dk_no_deferred);

  /* Use the main variant as the trampoline-map key: substituting a
     dependent label can yield a distinct cv-variant tree per instantiation
     for one and the same type, and keying on that would build a fresh
     trampoline for each.  Guard the lookup -- an ill-formed label reaches
     here with TREE_TYPE == error_mark_node, which is not a type node.  */
  tree label_type = TREE_TYPE (label);
  if (label_type && TYPE_P (label_type))
    label_type = TYPE_MAIN_VARIANT (label_type);
  if (!label_type || !CLASS_TYPE_P (label_type))
    {
      error_at (loc, "assertion-control expression must be a class type "
		"with a nested %<assertion_control_object%> type");
      CONTRACT_LABEL (contract) = NULL_TREE;
      return;
    }
  else
    {
      tree aco = lookup_member (label_type,
			       get_identifier ("assertion_control_object"),
			       /*protect=*/0, /*want_type=*/true,
			       tf_none);
      /* An inaccessible nested type is not one the concept can name, so such
	 a type is not an assertion-control object at all -- which, unlike an
	 inaccessible facet member, *is* ill-formed: the paper requires the
	 control object to satisfy this concept.  */
      if (!aco || aco == error_mark_node
	  || !label_facet_accessible_p (label_type, aco))
	{
	  error_at (loc, "type %qT does not satisfy "
		    "%<assertion_control_object%> "
		    "(missing or inaccessible nested type "
		    "%<assertion_control_object%>)",
		    label_type);
	  CONTRACT_LABEL (contract) = NULL_TREE;
	  return;
	}
    }

  /* Report members that look like they were meant to be facets but are not.
     Once per label type: the answer depends only on the type, and a label is
     typically named by many contracts.  */
  if (warn_contract_invalid_label_facet)
    {
      if (!near_miss_checked_types)
	near_miss_checked_types = hash_set<tree>::create_ggc (13);
      if (!near_miss_checked_types->add (label_type))
	{
	  static const char *const facets[] = {
	    "compute_semantic", "compute_comment", "compute_message",
	    "handle_contract_violation", "query"
	  };
	  for (unsigned i = 0; i < ARRAY_SIZE (facets); ++i)
	    warn_near_miss_facet (loc, label_facet_object (label), label_type,
				  facets[i]);
	  warn_near_miss_data_facet (loc, label_type, "allowed_semantics");
	  warn_near_miss_data_facet (loc, label_type, "group_names");
	}
    }

  /* Check for local_violation_label facet and generate trampoline
     if needed (P3400).  Keyed by label type so the same trampoline
     is reused across all contracts with the same label type.  */
  hash_map_maybe_create<hm_ggc> (local_violation_trampoline_map);

  if (!local_violation_trampoline_map->get (label_type))
    {
      tree hcv_id = get_identifier ("handle_contract_violation");
      tree hcv_fn = lookup_member (label_type, hcv_id,
				   /*protect=*/0, /*want_type=*/false,
				   tf_none);
      /* Clear hcv_fn rather than merely skipping the probe: the trampoline
	 is built by a separate `if' below that re-tests only hcv_fn, so
	 leaving it set would build a trampoline for a handler the probe
	 never vetted.  */
      if (hcv_fn && hcv_fn != error_mark_node
	  && !label_facet_accessible_p (label_type, hcv_fn))
	hcv_fn = NULL_TREE;
      if (hcv_fn && hcv_fn != error_mark_node)
	{
	  /* Test viability with the arguments the facet is probed with.  */
	  vec<tree, va_gc> *test_args
	    = build_facet_probe_args ("handle_contract_violation");
	  tree test_call = NULL_TREE;
	  if (test_args)
	    test_call = build_new_method_call (label_facet_object (label),
					       hcv_fn, &test_args, NULL_TREE,
					       LOOKUP_NORMAL, NULL, tf_none);
	  if (!test_call || test_call == error_mark_node)
	    hcv_fn = NULL_TREE;
	  /* The concept also requires the result to be void or convertible
	     to violation_handled; a handler returning anything else is not
	     the facet.  */
	  else if (!VOID_TYPE_P (TREE_TYPE (test_call)))
	    {
	      tree vh_type
		= find_std_contracts_type (get_identifier ("violation_handled"));
	      if (!vh_type || vh_type == error_mark_node
		  || !is_convertible (TREE_TYPE (test_call), vh_type))
		hcv_fn = NULL_TREE;
	    }
	}
      if (hcv_fn && hcv_fn != error_mark_node)
	{
	  tree resolved_fn = NULL_TREE;
	  tree trampoline
	    = build_local_violation_trampoline (label_type,
					       hcv_fn, &resolved_fn);
	  if (trampoline)
	    {
	      local_violation_trampoline_map->put (label_type,
						  trampoline);
	      if (resolved_fn)
		{
		  hash_map_maybe_create<hm_ggc> (local_violation_handler_fn_map);
		  local_violation_handler_fn_map->put (label_type,
						      resolved_fn);
		}
	    }
	}
    }


  /* Check for queryable_label facet and generate query trampoline
     if needed (P3400).  Keyed by label type so the same trampoline
     is reused across all contracts with the same label type.  */
  hash_map_maybe_create<hm_ggc> (query_trampoline_map);

  if (!query_trampoline_map->get (label_type))
    {
      tree query_id = get_identifier ("query");
      tree query_fn = lookup_member (label_type, query_id,
				    /*protect=*/0, /*want_type=*/false,
				    tf_none);
      if (query_fn && query_fn != error_mark_node
	  && label_facet_accessible_p (label_type, query_fn))
	{
	  /* Test viability: query(const void*, size_t) -> void*.  */
	  vec<tree, va_gc> *test_args = build_facet_probe_args ("query");
	  tree test_call = build_new_method_call (label_facet_object (label),
						  query_fn, &test_args,
						  NULL_TREE, LOOKUP_NORMAL,
						  NULL, tf_none);
	  /* labels::queryable_label requires exactly void*.  */
	  if (test_call && test_call != error_mark_node
	      && same_type_p (TREE_TYPE (test_call), ptr_type_node))
	    {
	      tree trampoline = build_query_trampoline (label_type);
	      if (trampoline)
		query_trampoline_map->put (label_type, trampoline);
	    }
	}
    }

  /* If the label needs its address taken (local handler or query
     trampoline) but is not already a VAR_DECL, materialize it as
     a static variable.  */
  bool needs_label_ptr
    = (local_violation_trampoline_map
       && local_violation_trampoline_map->get (label_type))
      || (query_trampoline_map
	  && query_trampoline_map->get (label_type));
  if (needs_label_ptr && !VAR_P (label))
    {
      /* The label is a prvalue -- pre<L{}> -- but the runtime
	 descriptor needs its address, so give it one.

	 Hand the unfolded expression to cp_finish_decl and let that do
	 the constant evaluation.  Pre-folding it here with
	 cxx_constant_value and passing the result in is wrong twice
	 over: on failure it returns error_mark_node and the whole
	 materialization is skipped in silence, leaving the label a
	 prvalue so the facet is never wired up at all; and on success
	 it produces a syntactic COMPOUND_LITERAL_P CONSTRUCTOR, which
	 store_init_value asserts against once pushdecl_top_level_and_finish
	 clears processing_template_decl underneath it.

	 Use the decl pushdecl_namespace_level hands back rather than the
	 one passed in; they need not be the same node.  */
      tree init = label;
      if (processing_template_decl)
	{
	  /* Parsing a template: the label expression is still in
	     template form, and pushdecl_top_level_and_finish clears
	     processing_template_decl underneath us, so cp_finish_decl
	     sees template trees with nothing left to say they are.  A
	     prvalue label then ICEs in store_init_value as the
	     undigested COMPOUND_LITERAL_P CONSTRUCTOR described above,
	     and a label with a dependent subtree ICEs in
	     dependent_type_p, reached from constant-evaluating that
	     initializer.

	     A dependent label cannot be materialized here in any case:
	     one TU-local constant cannot hold a distinct value per
	     instantiation.  Leave it a prvalue -- without a VAR_DECL
	     build_contract_data_block_ctor wires up no facet, so nothing
	     is mis-emitted meanwhile -- and let tsubst_contract
	     re-resolve it once it is concrete, materializing one constant
	     per instantiation.

	     Otherwise rebuild the expression outside the template, which
	     is what tsubst_contract does for the same reason.  */
	  if (instantiation_dependent_expression_p (init))
	    init = NULL_TREE;
	  else
	    init = instantiate_non_dependent_expr (init,
						   tf_warning_or_error);
	}
      if (init && init != error_mark_node)
	{
	  /* Make it a genuine constexpr constant, not merely a static one.
	     Facets consumed after this point read the label through
	     CONTRACT_LABEL, which is the variable built here, and a facet
	     whose result depends on the label's own state -- a
	     compute_semantic returning a data member, or an
	     allowed_semantics that is a non-static member -- then has to
	     read that variable during constant evaluation.  Left as a
	     plain static it is not readable there, cxx_constant_value
	     hands back error_mark_node, and the facet is silently dropped:
	     compute_semantic_core returns the semantic unchanged and the
	     allowed mask stays permissive, with no diagnostic either way.

	     The string facets do not need this only because grok_contract
	     applies them eagerly, before we get here.

	     Set on the label materialization alone rather than in
	     contracts_tu_local_named_var, whose other callers build
	     runtime data blocks that are not constant expressions.  */
	  tree const_type
	    = cp_build_qualified_type (label_type, TYPE_QUAL_CONST);
	  tree label_var = contracts_tu_local_named_var (
	    loc, "Lcontract_label", const_type);
	  DECL_CONTEXT (label_var) = NULL_TREE;
	  TREE_READONLY (label_var) = true;
	  DECL_DECLARED_CONSTEXPR_P (label_var) = true;
	  label_var = pushdecl_top_level_and_finish (label_var, init);
	  if (label_var && label_var != error_mark_node)
	    {
	      CONTRACT_LABEL (contract) = label_var;
	      label = label_var;
	    }
	}
    }

  /* Compute the flag-independent label restriction: start from the full
     semantic set (including the P4298 noexcept variants) and, when the label
     has an allowed_semantics facet, intersect it by probing each member.  The
     -fcontracts-allow-assume and -fcontracts-p4298 gates are applied later, at
     query construction (make_contract_query), so they are not baked in here.
     The base must include the noexcept variants so a label that explicitly
     allows them is not silently stripped of that capability (a facet-less
     contract gets them via the same WITH_EXTENSIONS default).  */
  uint16_t allowed_mask = CES_ALL_ALLOWED_WITH_EXTENSIONS;

  tree as_member = lookup_member (label_type,
				 get_identifier ("allowed_semantics"),
				 /*protect=*/0, /*want_type=*/false,
				 tf_none);
  /* The concept requires `__is_const (decltype (T::allowed_semantics))'.
     A `static constexpr' member is const-qualified already, and so is a
     plain `const' one; a non-const member is not a facet, and honouring
     it anyway made the front end narrow the semantic set for a label the
     library says has no allowed_semantics facet at all -- so a bare label
     and its combined form disagreed.  */
  tree as_decl = (as_member && as_member != error_mark_node
		  && BASELINK_P (as_member)
		  ? BASELINK_FUNCTIONS (as_member) : as_member);
  if (as_member && as_member != error_mark_node
      && label_facet_accessible_p (label_type, as_member)
      && as_decl && DECL_P (as_decl)
      && CP_TYPE_CONST_P (TREE_TYPE (as_decl)))
    {
      /* The facet is `evaluation_semantic_set ({label.allowed_semantics})'
	 ([support.contract.control.allowed]): build that set, as the
	 library concept does, and ask it -- a single semantic, or any type
	 convertible to the set, works that way, and only that way.  A
	 conversion or contains () that is not a constant expression is
	 diagnosed, once.  */
      tree set_type = lookup_std_contracts_type
	(get_identifier ("evaluation_semantic_set"));
      bool failed = !set_type || set_type == error_mark_node;
      /* In a template the label is not dependent (it would not be resolved
	 until its substitution otherwise), but an expression built there is
	 a template tree, which the constant evaluator does not take as it
	 is: build the facet's expressions, and evaluate them, outside it, on
	 the label's own value (GCC-916).  */
      tree clabel = (processing_template_decl
		     ? instantiate_non_dependent_expr (label, tf_none) : label);
      processing_template_decl_sentinel ptds;
      if (clabel == error_mark_node)
	failed = true;
      auto is_allowed = [&](uint16_t sem_val) -> bool
      {
	if (failed)
	  return true;
	tree as_val = finish_class_member_access_expr
	  (clabel, get_identifier ("allowed_semantics"), false,
	   tf_none);
	if (!as_val || as_val == error_mark_node)
	  return true;
	tree init = build_constructor_single (init_list_type_node,
					      NULL_TREE, as_val);
	tree set = build_functional_cast (loc, set_type,
					  build_tree_list (NULL_TREE, init),
					  tf_none);
	if (set == error_mark_node)
	  return true;
	tree contains = finish_class_member_access_expr
	  (set, get_identifier ("contains"), false, tf_none);
	tree arg = build_semantic_facet_arg (sem_val);
	if (!contains || contains == error_mark_node || !arg)
	  return true;
	releasing_vec args (make_tree_vector_single (arg));
	tree call = finish_call_expr (contains, &args, false, false,
				      tf_none);
	if (!call || call == error_mark_node)
	  return true;
	tree r = constant_facet_value (call, label_type,
				       "allowed_semantics", loc);
	if (r == error_mark_node)
	  {
	    failed = true;
	    return true;
	  }
	if (TREE_CODE (r) == INTEGER_CST)
	  return !integer_zerop (r);
	return true;
      };

      uint16_t restricted = 0;
      for (uint16_t sem = CES_IGNORE; sem <= CES_LAST; sem++)
	if ((allowed_mask & (1 << sem)) && is_allowed (sem))
	  restricted |= (1 << sem);
      allowed_mask = restricted;
      if (allowed_mask == 0)
	{
	  error_at (loc, "assertion-control label allows no "
		   "evaluation semantics");
	  CONTRACT_LABEL (contract) = NULL_TREE;
	  allowed_mask = CES_ALL_ALLOWED_WITH_EXTENSIONS;
	}
    }

  /* Store the label restriction; NULL_TREE means no restriction (the full
     set), so only store when the label narrowed it.  */
  if (allowed_mask != CES_ALL_ALLOWED_WITH_EXTENSIONS)
    CONTRACT_ALLOWED_MASK (contract)
      = build_int_cst (uint16_type_node, allowed_mask);
}

tree
grok_contract (tree contract_spec, tree result, cp_expr condition,
	       location_t loc, tree message, tree label,
	       tree requires_clause, char *comment)
{
  if (condition == error_mark_node)
    {
      free (comment);
      return error_mark_node;
    }

  tree_code code;
  contract_assertion_kind kind = CAK_INVALID;
  /* Both the standard spelling "contract_assert" and the extension spelling
     "__contract_assert" (see c-common.cc) tokenize to RID_CONTASSERT.  */
  if (IDENTIFIER_KEYWORD_P (contract_spec)
      && C_RID_CODE (contract_spec) == RID_CONTASSERT)
    {
      code = ASSERTION_STMT;
      kind = CAK_ASSERT;
    }
  else if (id_equal (contract_spec, "pre"))
    {
      code = PRECONDITION_STMT;
      kind = CAK_PRE;
    }
  else if (id_equal (contract_spec,"post"))
    {
      code = POSTCONDITION_STMT;
      kind = CAK_POST;
    }
  else
    gcc_unreachable ();

  /* Build the contract. The condition is added later.  In the case that
     the contract is deferred, result an plain identifier, not a result
     variable.  */
  tree contract;
  if (code != POSTCONDITION_STMT)
    {
      /* PRECONDITION_STMT / ASSERTION_STMT: 12 operands, all NULL_TREE.
	 Operands are filled in below and by ensure_evaluation_semantic.  */
      contract = build_nt (code,
			   NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE,
			   NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE,
			   NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE);
      TREE_TYPE (contract) = void_type_node;
      SET_EXPR_LOCATION (contract, loc);
    }
  else
    {
      /* POSTCONDITION_STMT: 15 operands; result (identifier) at op 12,
	 captures at op 13, result attributes at op 14.  */
      contract = build_nt (code,
			   NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE,
			   NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE,
			   NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE,
			   result, NULL_TREE, NULL_TREE);
      TREE_TYPE (contract) = void_type_node;
      SET_EXPR_LOCATION (contract, loc);
    }

  CONTRACT_LABEL (contract) = label;
  CONTRACT_REQUIRES_CLAUSE (contract) = requires_clause;

  /* Validate the label and compute its derived facets.  Deliberately not
     gated on whether CONDITION is deferred: for an in-class-defined member
     function, only the predicate is deferred, never the label, so the label
     is fully available here regardless.  (A template-dependent label is
     handled separately, by tsubst_contract calling this again once the
     label has been substituted to a concrete value.)  */
  resolve_contract_label (contract, label, loc);

  /* Determine the assertion kind.  */
  CONTRACT_ASSERTION_KIND (contract) = build_int_cst (uint16_type_node, kind);

  /* Validate and extract the user-defined diagnostic message (P3099) and apply
     the compute_message facet (P3400).  For a deferred contract the extraction
     is redone from the late-parse path once the condition is available.  */
  finish_contract_message (contract, message, condition, loc);

  /* If the contract is deferred, don't do anything with the condition.  */
  if (TREE_CODE (condition) == DEFERRED_PARSE)
    {
      free (comment);
      CONTRACT_CONDITION (contract) = condition;
      return contract;
    }

  /* Generate the comment from the original condition.  */
  CONTRACT_COMMENT (contract) = build_comment (condition, comment);

  /* Apply compute_comment facet (P3400) if present.  */
  CONTRACT_COMMENT (contract)
    = apply_label_string_facet (label, "compute_comment",
				CONTRACT_COMMENT (contract), loc);

  /* The condition is converted to bool.  */
  condition = finish_contract_condition (condition);

  if (condition == error_mark_node)
    return error_mark_node;

  CONTRACT_CONDITION (contract) = condition;

  /* A contract outside a class definition is parsed here and now rather than
     deferred, so this is where its predicate is complete.  There is no
     FUNCTION_DECL yet -- we are still in the declarator -- and none is
     needed: on this path any parameter belonging to a lambda's operator() is
     necessarily a lambda written inside the predicate, because a contract on
     a lambda is itself deferred and finishes in update_late_contract.  */
  if (POSTCONDITION_P (contract))
    {
      bool diagnosed;
      check_postcondition_param_odr_uses (condition, NULL_TREE, loc,
					  &diagnosed);
      if (diagnosed)
	CONTRACT_CONDITION (contract) = error_mark_node;
    }

  return contract;
}

/* Update condition of a late-parsed contract and postcondition variable,
   if any.  */

void
update_late_contract (tree contract, tree fndecl, tree result,
		      cp_expr condition, char *comment)
{
  if (TREE_CODE (contract) == POSTCONDITION_STMT)
    POSTCONDITION_IDENTIFIER (contract) = result;

  /* Generate the comment from the original condition.  */
  CONTRACT_COMMENT (contract) = build_comment (condition, comment);

  /* Apply compute_comment facet (P3400) if present.  */
  tree label = CONTRACT_LABEL (contract);
  CONTRACT_COMMENT (contract)
    = apply_label_string_facet (label, "compute_comment",
				CONTRACT_COMMENT (contract),
				EXPR_LOCATION (contract));

  /* The condition is converted to bool.  */
  condition = finish_contract_condition (condition);
  CONTRACT_CONDITION (contract) = condition;

  /* The predicate is complete, so [dcl.contract.func]/7 can be applied to it.
     This is the one place every late-parsed function contract passes through,
     which is what makes it the right hook: a postcondition with no
     result-name-introducer never reaches the result-variable rebuild.  */
  if (POSTCONDITION_P (contract))
    {
      bool diagnosed;
      check_postcondition_param_odr_uses (condition, fndecl,
					  EXPR_LOCATION (contract), &diagnosed);
      if (diagnosed)
	CONTRACT_CONDITION (contract) = error_mark_node;
    }
}

/* Returns the precondition function for FNDECL, or null if not set.  */

tree
get_precondition_function (tree fndecl)
{
  gcc_checking_assert (fndecl);
  tree *result = hash_map_safe_get (decl_pre_fn, fndecl);
  return result ? *result : NULL_TREE;
}

/* Returns the postcondition function for FNDECL, or null if not set.  */

tree
get_postcondition_function (tree fndecl)
{
  gcc_checking_assert (fndecl);
  tree *result = hash_map_safe_get (decl_post_fn, fndecl);
  return result ? *result : NULL_TREE;
}

/* Set the PRE and POST functions for FNDECL.  Note that PRE and POST can
   be null in this case.  If so the functions are not recorded.  Used by the
   modules code.  */

void
set_contract_functions (tree fndecl, tree pre, tree post)
{
  if (pre)
    set_precondition_function (fndecl, pre);

  if (post)
    set_postcondition_function (fndecl, post);
}


/* We're compiling the pre/postcondition function CONDFN; remap any FN
   contracts that match CODE and emit them.  */

static void
remap_and_emit_conditions (tree fn, tree condfn, tree_code code)
{
  gcc_assert (code == PRECONDITION_STMT || code == POSTCONDITION_STMT);
  tree contract_spec = get_fn_contract_specifiers (fn);
  if (!contract_spec)
    return;

  for (tree contract : tree_vec_range (contract_spec))
    if (TREE_CODE (contract) == code)
      {
	contract = copy_node (contract);
	if (CONTRACT_CONDITION (contract) != error_mark_node)
	  remap_contract (fn, condfn, contract, /*duplicate_p=*/false);
	emit_contract_statement (contract);
      }
}

/* Collect the capture struct ref PARM_DECLs from an outlined function.
   They are the last N params, where N is the number of active postconditions
   with captures in the original FNDECL.  Returns them in the same order
   as the postconditions with captures appear.  */

static auto_vec<tree, 4>
get_capture_struct_params (tree fndecl, tree outlined_fn)
{
  auto_vec<tree, 4> result;

  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return result;

  int num_struct_params = 0;
  for (tree contract : tree_vec_range (contracts))
    {
      if (active_postcondition_with_captures_p (contract, fndecl))
	num_struct_params++;
    }

  if (num_struct_params == 0)
    return result;

  auto_vec<tree, 8> all_params;
  for (tree p = DECL_ARGUMENTS (outlined_fn); p; p = DECL_CHAIN (p))
    all_params.safe_push (p);

  int start_idx = all_params.length () - num_struct_params;
  gcc_assert (start_idx >= 0);
  for (int i = start_idx; i < (int) all_params.length (); i++)
    result.safe_push (all_params[i]);

  return result;
}

/* Set up a copy_body_data for remapping from SRC function params to DST
   function params.  Only maps the "regular" params (skips __r and struct
   ref params).  Returns true if any mappings were inserted.  */

static bool
setup_param_remap (copy_body_data *id, hash_map<tree, tree> *decl_map,
		   tree src, tree dst)
{
  memset (id, 0, sizeof (*id));
  id->src_fn = src;
  id->dst_fn = dst;
  id->src_cfun = DECL_STRUCT_FUNCTION (src);
  id->decl_map = decl_map;
  id->copy_decl = copy_decl_no_change;
  id->transform_call_graph_edges = CB_CGE_DUPLICATE;
  id->transform_new_cfg = false;
  id->transform_return_to_modify = false;
  id->transform_parameter = true;
  id->regimplify = false;
  id->do_not_unshare = true;
  id->do_not_fold = true;
  id->eh_lp_nr = 0;

  /* Same by-reference adjustment as remap_contract: when the outlined
     function takes a parameter by reference, uses of the guarded function's
     parameter are value uses of *DP.  This path handles the
     postcondition-capture initialisers, which read the parameters at
     function entry.  */
  return map_corresponding_parms (id, src, dst, /*as_value=*/true);
}

/* Emit the body of an outlined __pre_fn when postcondition captures
   are present.  Interleaves precondition checks with capture initialization
   in lexical order (P3098).  Capture init writes to struct ref params.  */

static void
emit_outlined_pre_body (tree fndecl, tree pre_fn)
{
  auto_vec<tree, 4> struct_params
    = get_capture_struct_params (fndecl, pre_fn);
  int struct_idx = 0;

  copy_body_data id;
  hash_map<tree, tree> decl_map;
  setup_param_remap (&id, &decl_map, fndecl, pre_fn);

  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return;

  for (tree contract : tree_vec_range (contracts))
    {
      if (TREE_CODE (contract) == PRECONDITION_STMT)
	{
	  tree c = copy_node (contract);
	  if (CONTRACT_CONDITION (c) != error_mark_node)
	    remap_contract (fndecl, pre_fn, c, /*duplicate_p=*/false);
	  emit_contract_statement (c);
	}
      else if (active_postcondition_with_captures_p (contract, fndecl))
	{
	  tree struct_param = struct_params[struct_idx++];
	  tree struct_ref = convert_from_reference (struct_param);
	  tree struct_type = TREE_TYPE (struct_ref);
	  tree init_field = TYPE_FIELDS (struct_type);

	  /* Set __initialized = false.  */
	  tree init_ref = build3 (COMPONENT_REF, TREE_TYPE (init_field),
				  struct_ref, init_field, NULL_TREE);
	  finish_expr_stmt (cp_build_init_expr (init_ref, boolean_false_node));

	  /* As at the ordinary capture site: a dynamically selected
	     postcondition chooses its semantic here, into __sem, and
	     initializes the captures only if it is to be evaluated.  */
	  tree sel = NULL_TREE;
	  tree gate = NULL_TREE;
	  if (contract_dynamic_name (contract))
	    {
	      tree sem_field = TYPE_FIELDS (struct_type);
	      while (DECL_CHAIN (sem_field))
		sem_field = DECL_CHAIN (sem_field);
	      sel = build3 (COMPONENT_REF, TREE_TYPE (sem_field), struct_ref,
			    sem_field, NULL_TREE);
	      tree stmt = build_dynamic_selection_stmt (contract, sel);
	      remap_contract (fndecl, pre_fn, stmt, /*duplicate_p=*/false);
	      add_stmt (stmt);
	      gate = begin_if_stmt ();
	      finish_if_stmt_cond (dynamic_selection_initializes_captures (sel),
				   gate);
	    }

	  /* Try block: initialize each capture field.  As at the ordinary
	     capture site, this exists only to turn a throwing capture
	     initializer into a post_capture violation, so with -fno-exceptions
	     it is not built at all.  */
	  tree try_block = NULL_TREE;
	  if (flag_exceptions)
	    try_block = begin_try_block ();

	  tree field = DECL_CHAIN (init_field);
	  tree captures = POSTCONDITION_CAPTURES (contract);
	  for (tree cap = captures; cap;
	       cap = TREE_CHAIN (cap), field = DECL_CHAIN (field))
	    {
	      tree var = TREE_VALUE (cap);
	      tree cap_init = DECL_INITIAL (var);
	      if (!cap_init || cap_init == error_mark_node)
		continue;

	      /* Remap init expression from original function params to
		 pre function params.  */
	      tree remapped_init = unshare_expr (cap_init);
	      walk_tree (&remapped_init, copy_tree_body_r, &id, NULL);

	      tree union_type = TREE_TYPE (field);
	      tree union_member = TYPE_FIELDS (union_type);
	      tree union_ref = build3 (COMPONENT_REF, union_type,
				       struct_ref, field, NULL_TREE);
	      tree member_ref = build3 (COMPONENT_REF, TREE_TYPE (union_member),
					union_ref, union_member, NULL_TREE);
	      emit_postcondition_capture_init (member_ref, remapped_init);
	      push_capture_eh_cleanup (member_ref);
	    }

	  /* All inits succeeded: set __initialized = true.  */
	  finish_expr_stmt (cp_build_init_expr (init_ref, boolean_true_node));

	  if (flag_exceptions)
	    {
	      finish_try_block (try_block);

	      /* Catch (...): call violation handler.  */
	      tree handler = begin_handler ();
	      finish_handler_parms (NULL_TREE, handler);

	      emit_capture_exception_reaction (contract, fndecl, sel);

	      finish_handler (handler);
	      finish_handler_sequence (try_block);
	    }

	  if (gate)
	    {
	      finish_then_clause (gate);
	      finish_if_stmt (gate);
	    }

	  /* As at the inline capture site: an exception from a later
	     precondition or capture in this function destroys these
	     captures.  */
	  if (flag_exceptions)
	    {
	      tree cleanup = push_stmt_list ();
	      emit_capture_struct_dtors (struct_ref);
	      push_cleanup (NULL_TREE, pop_stmt_list (cleanup),
			    /*eh_only=*/true);
	    }
	}
    }
}

/* Walk_tree callback: replace capture VAR_DECLs with COMPONENT_REFs into
   the capture struct.  The DATA is a hash_map<tree,tree>* mapping capture
   VAR_DECL -> COMPONENT_REF.  */

static tree
remap_capture_vars_r (tree *here, int *do_subtree, void *data)
{
  hash_map<tree, tree> *cap_map = (hash_map<tree, tree> *) data;
  if (DECL_P (*here))
    {
      tree *mapped = cap_map->get (*here);
      if (mapped)
	{
	  *here = *mapped;
	  *do_subtree = 0;
	  return NULL_TREE;
	}
    }
  *do_subtree = 1;
  return NULL_TREE;
}

/* Emit the body of an outlined __post_fn when postcondition captures
   are present.  For each postcondition:
   - Without captures: remap and emit as usual
   - With captures: gate predicate on __initialized, remap capture VAR_DECLs
     to struct field COMPONENT_REFs, then emit  */

static void
emit_outlined_post_body (tree fndecl, tree post_fn)
{
  auto_vec<tree, 4> struct_params
    = get_capture_struct_params (fndecl, post_fn);
  int struct_idx = 0;

  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return;

  for (tree contract : tree_vec_range (contracts))
    {
      if (TREE_CODE (contract) != POSTCONDITION_STMT)
	continue;

      tree captures = POSTCONDITION_CAPTURES (contract);
      bool has_captures
	= active_postcondition_with_captures_p (contract, fndecl);

      tree c = copy_node (contract);
      if (!has_captures)
	{
	  if (CONTRACT_CONDITION (c) != error_mark_node)
	    remap_contract (fndecl, post_fn, c, /*duplicate_p=*/false);
	  emit_contract_statement (c);
	  continue;
	}

      tree struct_param = struct_params[struct_idx++];
      tree struct_ref = convert_from_reference (struct_param);
      tree struct_type = TREE_TYPE (struct_ref);
      tree init_field = TYPE_FIELDS (struct_type);

      /* Build capture VAR_DECL -> COMPONENT_REF mappings.  */
      hash_map<tree, tree> cap_map;
      tree field = DECL_CHAIN (init_field);
      for (tree cap = captures; cap;
	   cap = TREE_CHAIN (cap), field = DECL_CHAIN (field))
	{
	  tree var = TREE_VALUE (cap);
	  tree union_type = TREE_TYPE (field);
	  tree union_member = TYPE_FIELDS (union_type);
	  tree union_ref = build3 (COMPONENT_REF, union_type,
				   struct_ref, field, NULL_TREE);
	  tree member_ref = build3 (COMPONENT_REF, TREE_TYPE (union_member),
				    union_ref, union_member, NULL_TREE);
	  cap_map.put (var, member_ref);
	}
      /* Replace the captures before remapping the parameters, not after:
	 remap_contract copies a capture whose context is FNDECL (an in-class
	 member's contracts), and the copy no longer matches CAP_MAP.  The
	 fields are reached through STRUCT_PARAM, a POST_FN parameter the
	 remapping leaves alone.  Unshare first; the condition is still
	 FNDECL's own.  */
      if (CONTRACT_CONDITION (c) != error_mark_node)
	{
	  CONTRACT_CONDITION (c) = unshare_expr (CONTRACT_CONDITION (c));
	  walk_tree (&CONTRACT_CONDITION (c), remap_capture_vars_r, &cap_map,
		     NULL);
	  remap_contract (fndecl, post_fn, c, /*duplicate_p=*/false);
	}

      /* A dynamically selected postcondition dispatches on the semantic
	 its pre function chose into __sem.  */
      /* Gate predicate evaluation on __initialized.  */
      tree init_ref = build3 (COMPONENT_REF, TREE_TYPE (init_field),
			      struct_ref, init_field, NULL_TREE);
      tree cond = init_ref;
      if (contract_dynamic_name (contract))
	{
	  tree sem_field = init_field;
	  while (DECL_CHAIN (sem_field))
	    sem_field = DECL_CHAIN (sem_field);
	  tree sel = build3 (COMPONENT_REF, TREE_TYPE (sem_field), struct_ref,
			     sem_field, NULL_TREE);
	  note_dynamic_selection_site (c, /*select_p=*/false, sel);
	  cond = dynamic_selection_runs_check (cond, sel);
	}
      tree if_stmt = begin_if_stmt ();
      finish_if_stmt_cond (cond, if_stmt);
      emit_contract_statement (c);
      finish_then_clause (if_stmt);
      finish_if_stmt (if_stmt);
    }
}

/* Finish up the pre & post function definitions for a guarded FNDECL,
   and compile those functions all the way to assembler language output.  */

void
finish_function_outlined_contracts (tree fndecl)
{
  /* If the guarded func is either already decided to be ill-formed or is
     not yet complete return early.  */
  if (error_operand_p (fndecl)
      || !DECL_INITIAL (fndecl)
      || DECL_INITIAL (fndecl) == error_mark_node)
    return;

  /* If there are no contracts here, or we're building them in-line then we
     do not need to build the outlined functions.  */
  if (!handle_contracts_p (fndecl)
      || !flag_contract_checks_outlined)
    return;

  /* Skip outlined contract functions if there are no active callee-side
     contracts and this is not a wrapper function (which evaluates contracts
     caller-side).  The config system governs whether any contract is
     active.  */
  if (!contract_any_active_p (fndecl) && !DECL_CONTRACT_WRAPPER (fndecl))
    return;

  /* If either the pre or post functions are bad, don't bother emitting
     any contracts.  The program is already ill-formed.  */
  tree pre = DECL_PRE_FN (fndecl);
  tree post = DECL_POST_FN (fndecl);
  if (pre == error_mark_node || post == error_mark_node)
    return;

  /* We are generating code, deferred parses should be complete.  */
  tree contract_spec = get_fn_contract_specifiers (fndecl);
  gcc_checking_assert (!contract_any_deferred_p (contract_spec));

  int flags = SF_DEFAULT | SF_PRE_PARSED;

  bool has_caps = has_postcondition_captures_p (fndecl);

  if (pre && !DECL_INITIAL (pre))
    {
      DECL_PENDING_INLINE_P (pre) = false;
      start_preparsed_function (pre, DECL_ATTRIBUTES (pre), flags);
      if (has_caps)
	emit_outlined_pre_body (fndecl, pre);
      else
	remap_and_emit_conditions (fndecl, pre, PRECONDITION_STMT);
      finish_return_stmt (NULL_TREE);
      pre = finish_function (false);
      expand_or_defer_fn (pre);
    }

  if (post && !DECL_INITIAL (post))
    {
      DECL_PENDING_INLINE_P (post) = false;
      start_preparsed_function (post, DECL_ATTRIBUTES (post), flags);
      if (has_caps)
	emit_outlined_post_body (fndecl, post);
      else
	remap_and_emit_conditions (fndecl, post, POSTCONDITION_STMT);
      gcc_checking_assert (VOID_TYPE_P (TREE_TYPE (TREE_TYPE (post))));
      finish_return_stmt (NULL_TREE);
      post = finish_function (false);
      expand_or_defer_fn (post);
    }
}

/* ===== Code generation ===== */

/* Insert a BUILT_IN_OBSERVABLE_CHECKPOINT epoch marker.  */

static void
emit_builtin_observable_checkpoint ()
{
  tree fn = builtin_decl_explicit (BUILT_IN_OBSERVABLE_CHKPT);
  releasing_vec vec;
  fn = finish_call_expr (fn, &vec, false, false, tf_warning_or_error);
  finish_expr_stmt (fn);
}

/* P2900 quick_enforce: terminate the program via __builtin_trap () -- no
   handler is invoked.  The trap is emitted inline: it is noreturn and cold,
   so the code after a failed check relies on its predicate and the failure
   path costs no call.  That another translation unit may evaluate the same
   assertion differently is handled by marking the function as a refinable
   body (cp_genericize_r), not by hiding the trap.  */

static tree
build_quick_enforce_reaction (location_t loc)
{
  return build_call_expr_loc (loc, builtin_decl_explicit (BUILT_IN_TRAP), 0);
}

/* Lookup a name in std::contracts, or inject it.  */

static tree
lookup_std_contracts_type (tree name_id)
{
  tree id_ns = get_identifier ("contracts");
  tree ns = lookup_qualified_name (std_node, id_ns);

  tree res_type = error_mark_node;
  if (TREE_CODE (ns) == NAMESPACE_DECL)
    res_type = lookup_qualified_name
      (ns, name_id, LOOK_want::TYPE | LOOK_want::HIDDEN_FRIEND);

  if (TREE_CODE (res_type) == TYPE_DECL)
    res_type = TREE_TYPE (res_type);
  else
    {
      push_nested_namespace (std_node);
      push_namespace (id_ns, /*inline*/false);
      res_type = make_class_type (RECORD_TYPE);
      create_implicit_typedef (name_id, res_type);
      DECL_SOURCE_LOCATION (TYPE_NAME (res_type)) = BUILTINS_LOCATION;
      DECL_CONTEXT (TYPE_NAME (res_type)) = current_namespace;
      pushdecl_namespace_level (TYPE_NAME (res_type), /*hidden*/true);
      pop_namespace ();
      pop_nested_namespace (std_node);
    }
  return res_type;
}


/* Validate a user definition of ::handle_contract_violation per
   [basic.contract.handler] and [dcl.fct.def.replace].  Called from
   grokfndecl when a function with this name is declared at global scope.  */

void
check_handle_contract_violation (tree fndecl)
{
  location_t loc = DECL_SOURCE_LOCATION (fndecl);

  if (DECL_DECLARED_INLINE_P (fndecl))
    error_at (loc, "%<::handle_contract_violation%> must not be"
	      " declared %<inline%>");

  if (module_attach_p ())
    error_at (loc, "%<::handle_contract_violation%> must be attached"
	      " to the global module");

  if (DECL_LANGUAGE (fndecl) != lang_cplusplus)
    error_at (loc, "%<::handle_contract_violation%> must have"
	      " C++ language linkage");

  tree fntype = TREE_TYPE (fndecl);
  if (!same_type_p (TREE_TYPE (fntype), void_type_node))
    error_at (loc, "%<::handle_contract_violation%> must return %<void%>");

  tree parms = TYPE_ARG_TYPES (fntype);
  if (!parms || parms == void_list_node)
    {
      error_at (loc, "%<::handle_contract_violation%> must have a single"
		" parameter of type %<const std::contracts::"
		"contract_violation&%>");
      return;
    }

  tree parmtype = TREE_VALUE (parms);
  tree remaining = TREE_CHAIN (parms);
  if (remaining != void_list_node)
    {
      error_at (loc, "%<::handle_contract_violation%> must have a single"
		" parameter of type %<const std::contracts::"
		"contract_violation&%>");
      return;
    }

  if (!TYPE_REF_P (parmtype)
      || TYPE_REF_IS_RVALUE (parmtype))
    {
      error_at (loc, "parameter of %<::handle_contract_violation%> must be"
		" an lvalue reference to %<const std::contracts::"
		"contract_violation%>");
      return;
    }

  /* Exactly const: a const volatile reference is not the required type.  */
  tree reftype = TREE_TYPE (parmtype);
  if (cp_type_quals (reftype) != TYPE_QUAL_CONST)
    {
      error_at (loc, "parameter of %<::handle_contract_violation%> must be"
		" a reference to %<const std::contracts::"
		"contract_violation%>");
      return;
    }

  tree viol_type = lookup_std_contracts_type (
    get_identifier ("contract_violation"));
  if (!same_type_ignoring_top_level_qualifiers_p (reftype, viol_type))
    error_at (loc, "parameter of %<::handle_contract_violation%> must be"
	      " of type %<const std::contracts::contract_violation&%>");
}

/* Emit a C-linkage alias __handle_contract_violation for the user's
   ::handle_contract_violation, if defined in this TU.  A deleted definition
   is rejected here instead: [dcl.fct.def.replace] lists what a replacement
   function shall not be, and a deleted one replaces nothing (compare
   "::main cannot be deleted").  */

static void
maybe_emit_hcv_alias ()
{
  tree fnname = get_identifier ("handle_contract_violation");
  tree l = lookup_qualified_name (global_namespace, fnname,
				  LOOK_want::HIDDEN_FRIEND);
  tree fndecl = NULL_TREE;
  for (tree f: lkp_range (l))
    if (TREE_CODE (f) == FUNCTION_DECL && DECL_DELETED_FN (f))
      {
	error_at (DECL_SOURCE_LOCATION (f),
		  "%<::handle_contract_violation%> cannot be deleted");
	return;
      }
    else if (TREE_CODE (f) == FUNCTION_DECL && DECL_INITIAL (f) != NULL_TREE)
      {
	fndecl = f;
	break;
      }

  if (!fndecl || !TARGET_SUPPORTS_ALIASES)
    return;

  tree alias_id = get_identifier ("__handle_contract_violation");
  tree alias_decl = build_lang_decl (FUNCTION_DECL, alias_id,
				     TREE_TYPE (fndecl));
  DECL_SOURCE_LOCATION (alias_decl) = DECL_SOURCE_LOCATION (fndecl);
  TREE_PUBLIC (alias_decl) = true;
  DECL_EXTERNAL (alias_decl) = false;
  SET_DECL_LANGUAGE (alias_decl, lang_c);
  SET_DECL_ASSEMBLER_NAME (alias_decl, alias_id);

  cgraph_node::create_same_body_alias (alias_decl, fndecl);
}

static void emit_pending_weak_selectors ();

/* Emit any TU-level contract infrastructure (descriptor tables, etc.).
   Called at end of translation unit from cp_write_global_declarations.  */

void
maybe_emit_violation_handler_wrappers ()
{
  emit_pending_weak_selectors ();
  maybe_emit_hcv_alias ();
}

/* Early initialisation of types and functions we will use.  */
void
init_contracts ()
{
  init_terminate_fn ();
}

/* A label's facet trampolines are generated at the point the label is
   grokked, which can be in the middle of parsing something else entirely:
   inside a class body, inside a function body (for contract_assert), or
   during template instantiation.  Defining a function there disturbs parse
   state the enclosing construct is still using, and every piece of it has
   to be saved:

     - cfun and the statement-list stack, or the add_stmt that appends a
       contract_assert to the enclosing body finds an empty stmt_list_stack;
     - current_class_type/current_class_name, or finish_function pops a
       class scope that start_preparsed_function never pushed (the
       trampoline's DECL_CONTEXT is the global namespace), tripping the
       binding-level assertion in poplevel_class;
     - processing_template_decl, which a postcondition result name raises
       while its predicate is grokked, and which tsubst_contract raises for
       a deduced return type; start_preparsed_function would otherwise take
       its template path and leave current_function_decl unset.

   push_to_top_level saves all three -- it stacks cfun and installs a fresh
   scope_chain, so the class scope and processing_template_decl are cleared
   and restored together.  A trampoline is an ordinary non-template
   namespace-scope function in every case, so this is also semantically
   what we want.  */

namespace {

struct trampoline_scope
{
  trampoline_scope ()
  {
    /* Outside a function, keep function_depth nonzero so that we do not
       garbage-collect in the middle of an expression; within one,
       push_to_top_level's push_function_context already covers us.  */
    m_nested = (cfun != NULL);
    if (!m_nested)
      ++function_depth;
    push_to_top_level ();
  }

  ~trampoline_scope ()
  {
    pop_from_top_level ();
    if (!m_nested)
      --function_depth;
  }

private:
  bool m_nested;
};

} // anon namespace

/* Declare an artificial TU-local trampoline named NAME_PREFIX_<n>, of type
   FN_TYPE, taking the PARM_TYPES parameters, and open its body.

   Both P3400 trampolines -- the local violation handler and the queryable_label
   query -- are the same function shell around a two-or-three line body: same
   artificial FUNCTION_DECL setup, same parameter treatment, same internal
   linkage, same prologue.  Writing that out twice let the two drift, which they
   did.  The caller supplies the type and the body; everything structural is
   here.

   Returns the FUNCTION_DECL with its body open: the caller must emit statements
   and then finish with finish_contract_trampoline (or abandon it with
   abandon_contract_trampoline).  *BODY_OUT and *COMPOUND_OUT carry the state
   those two need.  */

static tree
begin_contract_trampoline (const char *name_prefix, int &counter,
			   tree fn_type, array_slice<tree> parm_types,
			   tree *body_out, tree *compound_out)
{
  char name[64];
  snprintf (name, sizeof (name), "%s_%d", name_prefix, counter++);

  location_t loc = BUILTINS_LOCATION;
  tree fn_decl = build_lang_decl_loc (loc, FUNCTION_DECL,
				      get_identifier (name), fn_type);
  DECL_CONTEXT (fn_decl) = FROB_CONTEXT (global_namespace);
  DECL_ARTIFICIAL (fn_decl) = true;
  DECL_INITIAL (fn_decl) = error_mark_node;
  DECL_RESULT (fn_decl) = NULL_TREE;

  tree first = NULL_TREE, prev = NULL_TREE;
  for (tree pt : parm_types)
    {
      tree p = cp_build_parm_decl (fn_decl, NULL_TREE, pt);
      TREE_USED (p) = true;
      DECL_READ_P (p) = true;
      if (prev)
	DECL_CHAIN (prev) = p;
      else
	first = p;
      prev = p;
    }
  DECL_ARGUMENTS (fn_decl) = first;

  TREE_PUBLIC (fn_decl) = false;
  DECL_EXTERNAL (fn_decl) = false;
  DECL_WEAK (fn_decl) = false;

  start_preparsed_function (fn_decl, NULL_TREE, SF_DEFAULT | SF_PRE_PARSED);
  *body_out = begin_function_body ();
  *compound_out = begin_compound_stmt (BCS_FN_BODY);
  return fn_decl;
}

/* Close a trampoline opened by begin_contract_trampoline and emit it.  */

static tree
finish_contract_trampoline (tree body, tree compound_stmt)
{
  finish_compound_stmt (compound_stmt);
  finish_function_body (body);
  tree fn_decl = finish_function (false);
  expand_or_defer_fn (fn_decl);
  return fn_decl;
}

/* Close a trampoline whose body could not be built, and emit nothing.  */

static void
abandon_contract_trampoline (tree body, tree compound_stmt)
{
  finish_compound_stmt (compound_stmt);
  finish_function_body (body);
  finish_function (false);
}

/* Build a trampoline function for local violation handlers (P3400).
   The generated function has signature:
     int __contract_local_handler_N (const void *label_ptr,
				     const void *violation_ptr)
   It casts label_ptr to LABEL_TYPE, casts violation_ptr to
   const contract_violation&, calls handle_contract_violation on the label,
   and returns 0 (not_handled) if void, or the int value of the result.

   HCV_FN is the result of the member lookup, which may still be an overload
   set; *RESOLVED_FN_OUT is set to the FUNCTION_DECL overload resolution
   actually picked, or NULL_TREE if that cannot be determined (a virtual
   handler, for instance).  The rethrow analysis needs the resolved callee, not
   the overload set.  */

static tree
build_local_violation_trampoline (tree label_type, tree hcv_fn,
				  tree *resolved_fn_out)
{
  *resolved_fn_out = NULL_TREE;

  /* Build function type: int(const void*, const void*)
     This matches __cxa_local_handler_fn_t in the ABI.  */
  tree const_void_ptr = build_pointer_type (
    cp_build_qualified_type (void_type_node, TYPE_QUAL_CONST));

  tree fn_type = build_function_type_list (integer_type_node,
					   const_void_ptr,
					   const_void_ptr,
					   NULL_TREE);

  /* Save the enclosing parse state for the duration; see
     trampoline_scope.  */
  trampoline_scope sentry;

  location_t loc = BUILTINS_LOCATION;
  static int local_handler_trampoline_counter = 0;
  tree body, compound_stmt;
  tree parm_types[] = { const_void_ptr, const_void_ptr };
  tree fn_decl
    = begin_contract_trampoline ("__contract_local_handler",
				 local_handler_trampoline_counter, fn_type,
				 parm_types, &body, &compound_stmt);

  tree parm_label_ptr = DECL_ARGUMENTS (fn_decl);
  tree parm_violation_ptr = DECL_CHAIN (parm_label_ptr);

  /* Cast: const LabelType& lbl = *(const LabelType*) label_ptr;

     Const, matching the query trampoline below and the concept:
     labels::local_violation_label requires `const _Tp __t', so a label whose
     handle_contract_violation is not const does not provide the facet and is
     rejected -- with -Wcontract-invalid-label-facet -- before any trampoline
     is built for it.  A non-const cast here would be the one place the two
     trampolines disagree, and would suggest, wrongly, that a non-const
     handler can get here.  Pinned by
     p3400-nonconst-handler-not-called.C.  */
  tree const_label_type = cp_build_qualified_type (label_type, TYPE_QUAL_CONST);
  tree label_ptr_type = build_pointer_type (const_label_type);
  tree cast_label = build1 (NOP_EXPR, label_ptr_type, parm_label_ptr);
  tree label_ref = cp_build_indirect_ref (loc, cast_label,
					  RO_UNARY_STAR, tf_warning_or_error);

  /* Cast: const contract_violation& v = *(const contract_violation*) ptr;  */
  tree cv_type
    = lookup_std_contracts_type (get_identifier ("contract_violation"));
  tree cv_const = cp_build_qualified_type (cv_type, TYPE_QUAL_CONST);
  tree cv_ptr_type = build_pointer_type (cv_const);
  tree cast_viol = build1 (NOP_EXPR, cv_ptr_type, parm_violation_ptr);
  tree violation_ref = cp_build_indirect_ref (loc, cast_viol, RO_UNARY_STAR,
					      tf_warning_or_error);

  /* Call: lbl.handle_contract_violation(violation);  */
  vec<tree, va_gc> *args = NULL;
  vec_safe_push (args, violation_ref);
  tree call = build_new_method_call (label_ref, hcv_fn, &args,
				     NULL_TREE, LOOKUP_NORMAL,
				     NULL, tf_none);

  if (!call || call == error_mark_node)
    {
      abandon_contract_trampoline (body, compound_stmt);
      return NULL_TREE;
    }

  /* Record which overload was picked; NULL for a virtual handler, which the
     rethrow analysis then declines to reason about.  */
  *resolved_fn_out = cp_get_callee_fndecl_nofold (call);

  tree ret_type = TREE_TYPE (call);
  if (VOID_TYPE_P (ret_type))
    {
      finish_expr_stmt (call);
      finish_return_stmt (integer_zero_node);
    }
  else
    {
      /* The facet returns something convertible to violation_handled, not
	 necessarily to int: convert to violation_handled first, then take
	 its value.  */
      tree result = call;
      tree vh_type
	= find_std_contracts_type (get_identifier ("violation_handled"));
      if (vh_type && vh_type != error_mark_node)
	result = perform_implicit_conversion (vh_type, result,
					      tf_warning_or_error);
      tree int_result = cp_convert (integer_type_node, result,
				    tf_warning_or_error);
      finish_return_stmt (int_result);
    }

  return finish_contract_trampoline (body, compound_stmt);
}

/* Build a trampoline function for the queryable_label facet (P3400).
   Generates:
     void* __contract_query_N(const void* label_ptr,
                              const void* key, size_t index)
     { return ((const LabelType*)label_ptr)->query(key, index); }
   This matches __cxa_query_fn_t in the ABI.  */

static tree
build_query_trampoline (tree label_type)
{
  /* Build function type: void*(const void*, const void*, size_t).  */
  tree const_void_ptr = build_pointer_type (
    cp_build_qualified_type (void_type_node, TYPE_QUAL_CONST));
  tree void_ptr = build_pointer_type (void_type_node);

  tree fn_type = build_function_type_list (void_ptr,
					   const_void_ptr,
					   const_void_ptr,
					   size_type_node,
					   NULL_TREE);

  /* Save the enclosing parse state for the duration; see
     trampoline_scope.  */
  trampoline_scope sentry;

  location_t loc = BUILTINS_LOCATION;
  static int query_trampoline_counter = 0;
  tree body, compound_stmt;
  tree parm_types[] = { const_void_ptr, const_void_ptr, size_type_node };
  tree fn_decl
    = begin_contract_trampoline ("__contract_query", query_trampoline_counter,
				 fn_type, parm_types, &body, &compound_stmt);

  tree parm_label_ptr = DECL_ARGUMENTS (fn_decl);
  tree parm_key = DECL_CHAIN (parm_label_ptr);
  tree parm_index = DECL_CHAIN (parm_key);

  /* Cast: const LabelType& lbl = *(const LabelType*) label_ptr;  */
  tree const_label_type = cp_build_qualified_type (label_type, TYPE_QUAL_CONST);
  tree label_ptr_type = build_pointer_type (const_label_type);
  tree cast_label = build1 (NOP_EXPR, label_ptr_type, parm_label_ptr);
  tree label_ref = cp_build_indirect_ref (loc, cast_label,
					  RO_UNARY_STAR, tf_warning_or_error);

  /* Call: lbl.query(key, index);  */
  tree query_id = get_identifier ("query");
  tree query_fns = lookup_member (label_type, query_id,
				  /*protect=*/0, /*want_type=*/false,
				  tf_warning_or_error);
  vec<tree, va_gc> *args = NULL;
  vec_safe_push (args, parm_key);
  vec_safe_push (args, parm_index);
  tree call = build_new_method_call (label_ref, query_fns, &args,
				     NULL_TREE, LOOKUP_NORMAL,
				     NULL, tf_none);

  if (!call || call == error_mark_node)
    {
      abandon_contract_trampoline (body, compound_stmt);
      return NULL_TREE;
    }

  /* Return the result (void*).  */
  finish_return_stmt (call);

  return finish_contract_trampoline (body, compound_stmt);
}

/* Build a named TU-local constant of TYPE.  */

static tree
contracts_tu_local_named_var (location_t loc, const char *name, tree type)
{
  tree var = build_decl (loc, VAR_DECL, NULL, type);
  DECL_NAME (var) = generate_internal_label (name);
  TREE_PUBLIC (var) = false;
  DECL_EXTERNAL (var) = false;
  TREE_STATIC (var) = true;
  DECL_ARTIFICIAL (var) = true;
  TREE_CONSTANT (var) = true;
  layout_decl (var, 0);
  return var;
}

/* Helper to replace references to dummy this parameters with references to
   the first argument of the FUNCTION_DECL DATA.  */

static tree
remap_dummy_this_1 (tree *tp, int *, void *data)
{
  if (!is_this_parameter (*tp))
    return NULL_TREE;

  /* is_this_parameter is true for a lambda's captured-'this' proxy as well
     as for a real 'this' PARM_DECL: the proxy is a VAR_DECL named 'this'
     whose DECL_VALUE_EXPR is already '__closure->__this'.  Rewriting that to
     DECL_ARGUMENTS -- which in a lambda's operator() is __closure, not an
     object of the enclosing class -- made a predicate read the closure as if
     it were that class.  The proxy needs no remapping; only the dummy 'this'
     of a contract parsed on a declaration does.  */
  if (TREE_CODE (*tp) != PARM_DECL)
    return NULL_TREE;

  tree fn = (tree)data;
  *tp = DECL_ARGUMENTS (fn);
  return NULL_TREE;
}

/* Replace all references to dummy this parameters in EXPR with references to
   the first argument of the FUNCTION_DECL FNDECL.  */

static void
remap_dummy_this (tree fndecl, tree *expr)
{
  walk_tree (expr, remap_dummy_this_1, fndecl, NULL);
}

/* Replace uses of user's placeholder var with the actual return value.  */

struct replace_tree
{
  tree from, to;
};

static tree
remap_retval_1 (tree *here, int *do_subtree, void *d)
{
  replace_tree *data = (replace_tree *) d;

  if (*here == data->from)
    {
      *here = data->to;
      *do_subtree = 0;
    }
  else
    *do_subtree = 1;
  return NULL_TREE;
}

static void
remap_retval (tree fndecl, tree contract)
{
  struct replace_tree data;
  data.from = POSTCONDITION_IDENTIFIER (contract);
  gcc_checking_assert (DECL_RESULT (fndecl));
  /* Read the returned object through the temporary apply_postconditions set
     up, when it decided one was needed; otherwise straight off DECL_RESULT.  */
  tree *temp = postcondition_retval_temps
	       ? postcondition_retval_temps->get (fndecl) : NULL;
  data.to = temp ? *temp : DECL_RESULT (fndecl);
  walk_tree (&CONTRACT_CONDITION (contract), remap_retval_1, &data, NULL);
}


/* === Contract violation data blocks ===

   The compiler emits __cxa_contract_data_block structs in .rodata and calls
   __cxa_contract_violation_* entry points defined in the contracts runtime
   (libcontracts, and libstdc++exp for C++).  See bits/contracts_abi.h for
   the ABI specification.  */

/* The data block variants.  Every block starts with the eight basic fields
   (descriptor, next, file, function, line, column, comment, message).  A
   "label" block adds a local handler, a "query" block a query function, and
   a "full" block both, the local handler first; each of those then ends with
   the label pointer.  Each variant has its own descriptor table, listing
   the IDs and offsets of the fields it carries.  */

enum contract_data_block_kind
{
  CDBK_BASIC,
  CDBK_LABEL,
  CDBK_QUERY,
  CDBK_FULL,
  CDBK_COUNT
};

static const struct contract_data_block_layout
{
  const char *block_type_name;
  const char *desc_type_name;
  const char *desc_var_name;
  bool local_handler;
  bool query_function;
} contract_data_block_layouts[CDBK_COUNT] = {
  { "__contract_data_block_basic", "__contract_desc_basic",
    "Lcontract_desc_basic", false, false },
  { "__contract_data_block_label", "__contract_desc_label",
    "Lcontract_desc_label", true, false },
  { "__contract_data_block_query", "__contract_desc_query",
    "Lcontract_desc_query", false, true },
  { "__contract_data_block_full", "__contract_desc_full",
    "Lcontract_desc_full", true, true },
};

/* The number of basic fields.  */
#define CONTRACT_DATA_BLOCK_BASIC_FIELDS 8

/* The data block RECORD_TYPE of each variant.  */
static GTY(()) tree contract_data_block_types[CDBK_COUNT];

/* The descriptor table static const instance of each variant (one per
   TU).  */
static GTY(()) tree contract_desc_vars[CDBK_COUNT];

/* Build a RECORD_TYPE from parallel arrays of types and names.  */

static tree
build_record_type_from_arrays (const char *struct_name,
			       const tree *types, const char **names,
			       unsigned count)
{
  tree fields = NULL_TREE;
  for (unsigned i = 0; i < count; i++)
    {
      tree next = build_decl (BUILTINS_LOCATION, FIELD_DECL,
			      get_identifier (names[i]), types[i]);
      DECL_CHAIN (next) = fields;
      fields = next;
    }

  iloc_sentinel ils (input_location);
  input_location = BUILTINS_LOCATION;
  tree type = cxx_make_type (RECORD_TYPE);
  finish_builtin_struct (type, struct_name, fields, NULL_TREE);
  DECL_ARTIFICIAL (TYPE_NAME (type)) = true;
  TYPE_ARTIFICIAL (type) = true;
  type = cp_build_qualified_type (type, TYPE_QUAL_CONST);
  return type;
}

/* Initialize the data block RECORD_TYPEs.  */

static void
init_contract_data_block_types ()
{
  if (contract_data_block_types[CDBK_BASIC])
    return;

  for (unsigned k = 0; k < CDBK_COUNT; k++)
    {
      const contract_data_block_layout &layout
	= contract_data_block_layouts[k];
      auto_vec<tree, 11> types;
      auto_vec<const char *, 11> names;
      auto add = [&] (tree type, const char *name)
	{
	  types.safe_push (type);
	  names.safe_push (name);
	};
      add (ptr_type_node, "_descriptor");
      add (ptr_type_node, "_next");
      add (const_string_type_node, "_file");
      add (const_string_type_node, "_function");
      add (unsigned_type_node, "_line");
      add (unsigned_type_node, "_column");
      add (const_string_type_node, "_comment");
      add (const_string_type_node, "_message");
      gcc_checking_assert (types.length ()
			   == CONTRACT_DATA_BLOCK_BASIC_FIELDS);
      if (layout.local_handler)
	add (ptr_type_node, "_local_handler");
      if (layout.query_function)
	add (ptr_type_node, "_query_function");
      if (layout.local_handler || layout.query_function)
	add (ptr_type_node, "_label_ptr");
      contract_data_block_types[k]
	= build_record_type_from_arrays (layout.block_type_name,
					 types.address (), names.address (),
					 types.length ());
    }
}

/* Build a descriptor table RECORD_TYPE with NUM_ENTRIES entries.  Layout:
   header (u8), num_entries (u8), fid[N] (u8 each), pad, off[N] (uintptr
   each).  */

static tree
build_descriptor_table_type (const char *name, unsigned num_entries)
{
  unsigned num_fields = 2 + num_entries + num_entries;
  auto_vec<tree> types (num_fields);
  auto_vec<const char *> names (num_fields);

  /* header and num_entries.  */
  types.safe_push (unsigned_char_type_node);
  names.safe_push ("_header");
  types.safe_push (unsigned_char_type_node);
  names.safe_push ("_num_entries");

  /* Field IDs.  */
  char fid_name[32];
  for (unsigned i = 0; i < num_entries; i++)
    {
      snprintf (fid_name, sizeof (fid_name), "_fid_%u", i);
      types.safe_push (unsigned_char_type_node);
      names.safe_push (IDENTIFIER_POINTER (get_identifier (fid_name)));
    }

  /* Offset values (uintptr_t).  GCC may add padding before these.  */
  char off_name[32];
  for (unsigned i = 0; i < num_entries; i++)
    {
      snprintf (off_name, sizeof (off_name), "_off_%u", i);
      types.safe_push (size_type_node);
      names.safe_push (IDENTIFIER_POINTER (get_identifier (off_name)));
    }

  return build_record_type_from_arrays (name, types.address (),
					names.address (), num_fields);
}

/* Append VAL to ELTS as the initializer of *FIELD, and advance *FIELD to the
   next aggregate field.  */

static void
append_field_init (vec<constructor_elt, va_gc> *&elts, tree *field, tree val)
{
  CONSTRUCTOR_APPEND_ELT (elts, *field, val);
  *field = next_aggregate_field (DECL_CHAIN (*field));
}

/* Get the Nth field of a RECORD_TYPE, walking DECL_CHAIN.  */

static tree
get_nth_field (tree record_type, unsigned n)
{
  tree f = next_aggregate_field (TYPE_FIELDS (record_type));
  for (unsigned i = 0; f && i < n; i++)
    f = next_aggregate_field (DECL_CHAIN (f));
  return f;
}

/* The field IDs the descriptor of the data block variant KIND lists, in
   FIDS, and the index of each one's field in the data block, in FIELDS.
   Return the count.  */

static unsigned
contract_data_block_entries (contract_data_block_kind kind,
			     unsigned char fids[6], unsigned fields[6])
{
  const contract_data_block_layout &layout = contract_data_block_layouts[kind];
  unsigned n = 0;
  auto add = [&] (unsigned char fid, unsigned field)
    {
      fids[n] = fid;
      fields[n] = field;
      n++;
    };
  add (CXA_FIELD_SOURCE_LOCATION, 2);	/* _file */
  add (CXA_FIELD_COMMENT, 6);		/* _comment */
  add (CXA_FIELD_MESSAGE, 7);		/* _message */
  unsigned next = CONTRACT_DATA_BLOCK_BASIC_FIELDS;
  if (layout.local_handler)
    add (CXA_FIELD_LOCAL_HANDLER, next++);
  if (layout.query_function)
    add (CXA_FIELD_QUERY_FUNCTION, next++);
  if (layout.local_handler || layout.query_function)
    add (CXA_FIELD_LABEL_PTR, next++);
  return n;
}

/* Initialize and emit descriptor table constants for this TU.
   Call once per TU, when contracts are first seen.  */

static void
init_contract_descriptor_tables ()
{
  if (contract_desc_vars[CDBK_BASIC])
    return;

  init_contract_data_block_types ();
  tree uchar = unsigned_char_type_node;

  for (unsigned k = 0; k < CDBK_COUNT; k++)
    {
      contract_data_block_kind kind = (contract_data_block_kind) k;
      const contract_data_block_layout &layout
	= contract_data_block_layouts[k];
      unsigned char fids[6];
      unsigned fields[6];
      unsigned n = contract_data_block_entries (kind, fids, fields);
      tree desc_type = build_descriptor_table_type (layout.desc_type_name, n);

      /* Fields: header, num_entries, fid[N], off[N].  */
      vec<constructor_elt, va_gc> *elts = NULL;
      tree f = next_aggregate_field (TYPE_FIELDS (desc_type));
      append_field_init (elts, &f, build_int_cst (uchar, CXA_DESC_HEADER_BYTE));
      append_field_init (elts, &f, build_int_cst (uchar, n));
      for (unsigned i = 0; i < n; i++)
	append_field_init (elts, &f, build_int_cst (uchar, fids[i]));
      for (unsigned i = 0; i < n; i++)
	append_field_init (elts, &f,
			   byte_position (get_nth_field
					    (contract_data_block_types[k],
					     fields[i])));
      tree ctor = build_constructor (desc_type, elts);
      TREE_CONSTANT (ctor) = true;
      TREE_READONLY (ctor) = true;

      tree var = contracts_tu_local_named_var (BUILTINS_LOCATION,
					       layout.desc_var_name,
					       desc_type);
      DECL_INITIAL (var) = ctor;
      varpool_node::finalize_decl (var);
      /* These descriptor tables are shared TU-local statics, finalized lazily
	 when the first contract data block in the TU is built.  That first
	 block may belong to an inline/COMDAT library function (e.g. under a
	 catch-all "kind: implicit" configuration that matches implicit
	 assertions in <contracts>' own inline code); if all such early
	 referrers are later reclaimed by symtab_remove_unreachable_nodes, the
	 descriptor would be removed too -- yet a middle-end check instrumented
	 later in pass_ubsan can still reference it, leaving a dangling
	 reference at link time.  Force the descriptors to be emitted so a late
	 referrer always resolves.  */
      varpool_node::get (var)->force_output = true;
      contract_desc_vars[k] = var;
    }
}

/* Build a data block constructor for CONTRACT.
   Returns a tree for the constructor and sets *out_type to the type used.  */

static tree
build_contract_data_block_ctor (tree contract, tree *out_type)
{
  init_contract_descriptor_tables ();

  location_t loc = EXPR_LOCATION (contract);

  /* Determine if this assertion has a label with facets needing runtime
     data.  */
  tree label = CONTRACT_LABEL (contract);
  bool has_label = (label && label != error_mark_node
		    && TREE_TYPE (label)
		    && !type_dependent_expression_p (label));
  bool has_handler = false;
  bool has_query = false;
  tree label_ptr_val = build_zero_cst (ptr_type_node);
  tree local_handler_val = build_zero_cst (ptr_type_node);
  tree query_function_val = build_zero_cst (ptr_type_node);

  if (has_label)
    {
      /* Must match the key resolve_contract_label used.  */
      tree label_type = TYPE_MAIN_VARIANT (TREE_TYPE (label));
      if (local_violation_trampoline_map && VAR_P (label))
	{
	  tree *trampoline_p = local_violation_trampoline_map->get (label_type);
	  if (trampoline_p)
	    {
	      has_handler = true;
	      label_ptr_val = build_address (label);
	      local_handler_val = build_address (*trampoline_p);
	    }
	}
      if (query_trampoline_map && VAR_P (label))
	{
	  tree *trampoline_p = query_trampoline_map->get (label_type);
	  if (trampoline_p)
	    {
	      has_query = true;
	      if (!has_handler)
		label_ptr_val = build_address (label);
	      query_function_val = build_address (*trampoline_p);
	    }
	}
    }

  /* Select the block variant.  */
  contract_data_block_kind kind
    = (has_handler ? (has_query ? CDBK_FULL : CDBK_LABEL)
       : has_query ? CDBK_QUERY : CDBK_BASIC);
  tree block_type = contract_data_block_types[kind];

  /* Get source location components.  */
  /* No function for a P3100 check in a namespace-scope or default member
     initializer: the function name is then empty.  */
  tree fndecl = current_function_decl;
  if (fndecl && (DECL_IS_PRE_FN_P (fndecl) || DECL_IS_POST_FN_P (fndecl)))
    fndecl = get_orig_for_outlined (fndecl);
  if (fndecl && DECL_IS_WRAPPER_FN_P (fndecl))
    fndecl = get_orig_func_for_wrapper (fndecl);

  const char *file = LOCATION_FILE (loc);
  if (!file)
    file = "";
  tree file_str = build_string_literal (file);

  const char *funcname = "";
  if (fndecl)
    funcname = cxx_printable_name (fndecl, 2);
  tree func_str = build_string_literal (funcname);

  tree line_val = build_int_cst (unsigned_type_node, LOCATION_LINE (loc));
  tree col_val = build_int_cst (unsigned_type_node, LOCATION_COLUMN (loc));

  tree comment = CONTRACT_COMMENT (contract);
  if (!comment)
    comment = build_string_literal ("");

  tree message = CONTRACT_MESSAGE (contract);
  if (message)
    message = build_string_literal (TREE_STRING_LENGTH (message),
				    TREE_STRING_POINTER (message));
  else
    message = build_zero_cst (const_string_type_node);

  /* Build the constructor.  */
  vec<constructor_elt, va_gc> *elts = NULL;
  tree f = next_aggregate_field (TYPE_FIELDS (block_type));
  append_field_init (elts, &f, build_address (contract_desc_vars[kind]));
  append_field_init (elts, &f, build_zero_cst (ptr_type_node));  /* next */
  append_field_init (elts, &f, file_str);
  append_field_init (elts, &f, func_str);
  append_field_init (elts, &f, line_val);
  append_field_init (elts, &f, col_val);
  append_field_init (elts, &f, comment);
  append_field_init (elts, &f, message);
  if (has_handler)
    append_field_init (elts, &f, local_handler_val);
  if (has_query)
    append_field_init (elts, &f, query_function_val);
  if (has_handler || has_query)
    append_field_init (elts, &f, label_ptr_val);
  gcc_checking_assert (!f);
  tree ctor = build_constructor (block_type, elts);

  TREE_READONLY (ctor) = true;
  TREE_CONSTANT (ctor) = true;

  *out_type = block_type;
  return ctor;
}

/* Create the read-only data block constant for CONTRACT and return its
   address.  */

static tree
build_contract_data_block_address (tree contract)
{
  tree block_type;
  tree ctor = build_contract_data_block_ctor (contract, &block_type);
  tree var = contracts_tu_local_named_var (EXPR_LOCATION (contract),
					  "Lcontract_data", block_type);
  DECL_INITIAL (var) = ctor;
  varpool_node::finalize_decl (var);
  return build_address (var);
}

/* Return the entry point name for the given combination.  The noexcept
   semantics have the _noexcept-suffixed entry points.  */

static tree
get_cxa_entry_point_name (contract_assertion_kind kind,
			  contract_evaluation_semantic semantic,
			  int mode)
{
  const char *kind_str;
  switch (kind)
    {
    case CAK_PRE: kind_str = "pre"; break;
    case CAK_POST: kind_str = "post"; break;
    case CAK_ASSERT: kind_str = "assert"; break;
    case CAK_POST_CAPTURE: kind_str = "post_capture"; break;
    case CAK_IMPLICIT: kind_str = "implicit"; break;
    default: gcc_unreachable ();
    }

  const char *sem_str;
  switch (semantic)
    {
    case CES_ENFORCE: sem_str = "enforce"; break;
    case CES_OBSERVE: sem_str = "observe"; break;
    case CES_NOEXCEPT_ENFORCE: sem_str = "noexcept_enforce"; break;
    case CES_NOEXCEPT_OBSERVE: sem_str = "noexcept_observe"; break;
    default: gcc_unreachable ();
    }

  const char *mode_str;
  switch (mode)
    {
    case CDM_PREDICATE_FALSE: mode_str = "pf"; break;
    case CDM_EVAL_EXCEPTION: mode_str = "ex"; break;
    default: gcc_unreachable ();
    }

  char buf[128];
  if (contract_semantic_noexcept_p (semantic))
    snprintf (buf, sizeof (buf),
	      "__cxa_contract_violation_%s_%s_%s_noexcept",
	      kind_str, sem_str, mode_str);
  else
    snprintf (buf, sizeof (buf),
	      "__cxa_contract_violation_%s_%s_%s",
	      kind_str, sem_str, mode_str);

  return get_identifier (buf);
}

/* Cached entry point declarations, keyed by name.  Keyed on the identifier,
   not the string: a GTY string key is a pointer the PCH writer cannot
   relocate.  */
static GTY(()) hash_map<tree, tree, pch_traits> *cxa_entry_point_cache;

/* Declare (or return cached) a __cxa_contract_violation_* entry point.  */

static tree
declare_cxa_entry_point (contract_assertion_kind kind,
			 contract_evaluation_semantic semantic,
			 int mode)
{
  tree name = get_cxa_entry_point_name (kind, semantic, mode);

  hash_map_maybe_create<hm_ggc> (cxa_entry_point_cache, 16);

  tree *cached = cxa_entry_point_cache->get (name);
  if (cached)
    return *cached;

  bool is_noreturn = (semantic == CES_ENFORCE
		      || semantic == CES_NOEXCEPT_ENFORCE);
  tree fntype = build_function_type_list (void_type_node,
					  ptr_type_node, NULL_TREE);
  if (contract_semantic_noexcept_p (semantic))
    fntype = build_exception_variant (fntype, NULL_TREE);
  tree fndecl = build_lang_decl (FUNCTION_DECL, name, fntype);
  /* A namespace-scope entity like any other, so that a module streaming a
     body that calls it can name it.  */
  DECL_CONTEXT (fndecl) = FROB_CONTEXT (global_namespace);
  SET_DECL_LANGUAGE (fndecl, lang_c);
  TREE_PUBLIC (fndecl) = true;
  DECL_EXTERNAL (fndecl) = true;
  DECL_ARTIFICIAL (fndecl) = true;
  if (is_noreturn)
    TREE_THIS_VOLATILE (fndecl) = true;

  tree parms = build_decl (BUILTINS_LOCATION, PARM_DECL,
			   NULL_TREE, ptr_type_node);
  DECL_CONTEXT (parms) = fndecl;
  DECL_ARGUMENTS (fndecl) = parms;

  cxa_entry_point_cache->put (name, fndecl);
  return fndecl;
}

/* ------------------------------------------------------------------------
   Bypassing a rethrowing local violation handler (quality of
   implementation, -fcontract-bypass-rethrowing-local-handler).

   A P3400 local violation handler that responds to an evaluation_exception
   detection by rethrowing the in-flight exception makes the try/catch we wrap
   around a possibly-throwing predicate pure overhead: the exception is caught
   only to be handed to a handler that throws it straight back out.  This
   analysis recognizes that shape so the caller can skip emitting the
   try/catch and let the predicate's exception propagate on its own.

   Equivalence rests on the local handler running before the global one and
   short-circuiting it (libcontracts/dispatch.c): if the local handler
   rethrows, nothing else observable happens between the catch and the
   rethrow, and no violation is ever reported.  It holds only for the enforce
   and observe semantics -- quick_enforce calls no handler, and the P4298
   noexcept_* semantics exist precisely to guarantee nothing propagates.

   The walk follows calls, which is what lets it see through delegation: a
   handler that calls a helper whose body is just `throw;', and the
   __combined_label handler that forwards to the component labels, both come
   out of the same mechanism rather than being special-cased.  Following a
   call needs both possible answers -- the callee always rethrows, or it
   always returns having done nothing -- because in the second case the walk
   has to carry on in the caller.  For a combined label that means the
   optimization applies when the rethrowing component is reached without any
   earlier component doing anything at all, and not otherwise, which is
   exactly the condition under which skipping the handler is sound.

   Everything here is conservative: any construct the walk does not model
   makes it answer "no", leaving the try/catch in place.  Keeping the whole
   analysis behind one predicate lets it grow more capable without spreading
   through the emitter.
   ------------------------------------------------------------------------ */

namespace {

/* An abstract value tracked while walking a handler body.  */

enum aval_kind
{
  AV_UNKNOWN,		/* Nothing known.  */
  AV_CONST,		/* A known integer, enumeration or boolean value.  */
  AV_CURRENT_EXCEPTION	/* The result of std::current_exception ().  */
};

struct aval
{
  aval_kind kind;
  HOST_WIDE_INT val;
};

inline aval
av_unknown ()
{
  aval a = { AV_UNKNOWN, 0 };
  return a;
}

inline aval
av_const (HOST_WIDE_INT v)
{
  aval a = { AV_CONST, v };
  return a;
}

inline aval
av_current_exception ()
{
  aval a = { AV_CURRENT_EXCEPTION, 0 };
  return a;
}

/* How control left a statement, under the analysis assumption.

   RO_RETURNED is distinct from RO_FAIL because the two mean different things
   depending on whose frame we are in.  For the handler itself, returning is a
   failure: it did not rethrow.  For a function the handler called, returning
   is a success of sorts -- the call completed without doing anything
   observable, so the walk of the caller carries on past it.  */

enum rethrow_outcome
{
  RO_FALLTHROUGH,	/* Control continues with the next statement.  */
  RO_RETHROWN,		/* Control left by rethrowing the in-flight
			   exception.  */
  RO_RETURNED,		/* Control returned normally, having done nothing
			   observable.  */
  RO_BREAK,		/* Control left the innermost enclosing switch by
			   a break.  */
  RO_FAIL		/* Could not be analysed, or something observable
			   happened.  */
};

/* How deep the walk will follow calls before giving up.  A handler that
   delegates more than this far is not a shape worth proving, and the limit
   doubles as the termination guard for mutual recursion.  */

constexpr int RETHROW_MAX_DEPTH = 8;

/* Strip the conversions and address/indirection operators that stand between
   a use of a declaration and the declaration itself.  */

tree
strip_to_decl (tree t)
{
  while (t
	 && (CONVERT_EXPR_P (t)
	     || TREE_CODE (t) == NON_LVALUE_EXPR
	     || TREE_CODE (t) == ADDR_EXPR
	     || TREE_CODE (t) == INDIRECT_REF))
    t = TREE_OPERAND (t, 0);
  return t;
}

/* True if CALL (a CALL_EXPR or AGGR_INIT_EXPR) calls a namespace-scope
   function in namespace std named NAME.  */

bool
calls_std_fn_p (tree call, const char *name)
{
  tree fn = cp_get_callee_fndecl_nofold (call);
  if (!fn || TREE_CODE (fn) != FUNCTION_DECL || !DECL_NAME (fn))
    return false;
  if (!id_equal (DECL_NAME (fn), name))
    return false;
  return decl_in_std_namespace_p (fn);
}

/* One analysis of one handler body, under one (semantic, kind) pair.  The
   walk assumes the violation was detected as CDM_EVAL_EXCEPTION.  */

class rethrow_analysis
{
public:
  rethrow_analysis (tree violation_parm, tree violation_type,
		    contract_evaluation_semantic semantic,
		    contract_assertion_kind kind,
		    int depth = 0)
    : m_violation_parm (violation_parm), m_violation_type (violation_type),
      m_semantic (semantic), m_kind (kind), m_depth (depth),
      m_returned (av_unknown ())
  {}

  rethrow_outcome walk_stmt (tree t);

  /* Valid after walk_stmt returned RO_RETURNED: what the function returned,
     as far as the abstract domain could tell.  */
  aval returned_value () const { return m_returned; }

private:
  rethrow_outcome walk_switch (tree t);
  aval eval (tree t);
  bool accessor_value (tree call, aval *out);
  rethrow_outcome call_outcome (tree call, aval *value_out);

  /* Evaluate T for its value, insisting that getting there costs nothing
     observable.  False means the expression is not something the domain can
     account for, and the statement containing it must not be walked past.  */
  bool eval_pure (tree t, aval *out)
  {
    m_impure = false;
    m_rethrew = false;
    *out = eval (t);
    return !m_impure;
  }

  /* Evaluate T where a value is wanted and control is expected to carry on.
     RO_FALLTHROUGH means *OUT holds it; RO_RETHROWN means evaluating T never
     produced a value at all, because something it called rethrew.  */
  rethrow_outcome eval_value (tree t, aval *out)
  {
    if (eval_pure (t, out))
      return RO_FALLTHROUGH;
    return m_rethrew ? RO_RETHROWN : RO_FAIL;
  }

  tree m_violation_parm;
  tree m_violation_type;
  contract_evaluation_semantic m_semantic;
  contract_assertion_kind m_kind;
  int m_depth;
  aval m_returned;

  /* Set by eval when it meets something it cannot account for.  AV_UNKNOWN
     alone does not mean "unmodelled" -- reading an untracked local yields an
     unknown value from a perfectly pure expression -- so a caller that is
     willing to carry on with an unknown value still has to know whether
     getting there cost anything observable.  */
  bool m_impure = false;

  /* Set alongside m_impure when the reason no value came back is that a call
     inside the expression always rethrows.  */
  bool m_rethrew = false;

  /* Local scalar VAR_DECL -> abstract value.  */
  hash_map<tree, aval> m_env;

  /* The label a break from the switch being walked jumps to, if any.  */
  tree m_break_label = NULL_TREE;
};

/* If CALL invokes one of the contract_violation accessors whose result is
   known at the point the check is emitted, store that value in *OUT and
   return true.  The call must be on the handler's own violation parameter:
   a different contract_violation object tells us nothing.  */

bool
rethrow_analysis::accessor_value (tree call, aval *out)
{
  tree fn = cp_get_callee_fndecl_nofold (call);
  if (!fn || TREE_CODE (fn) != FUNCTION_DECL || !DECL_NAME (fn))
    return false;

  tree ctx = DECL_CONTEXT (fn);
  if (!ctx || !TYPE_P (ctx) || TYPE_MAIN_VARIANT (ctx) != m_violation_type)
    return false;

  if (TREE_CODE (call) != CALL_EXPR || call_expr_nargs (call) < 1)
    return false;
  if (strip_to_decl (CALL_EXPR_ARG (call, 0)) != m_violation_parm)
    return false;

  tree name = DECL_NAME (fn);
  if (id_equal (name, "detection_mode"))
    *out = av_const (CDM_EVAL_EXCEPTION);
  else if (id_equal (name, "semantic"))
    *out = av_const (m_semantic);
  else if (id_equal (name, "kind"))
    *out = av_const (m_kind);
  else if (id_equal (name, "is_terminating"))
    /* Of the two semantics this analysis runs for, only enforce
       terminates.  */
    *out = av_const (m_semantic == CES_ENFORCE);
  else
    return false;

  return true;
}

/* Walk into CALL's callee and report how control leaves the call.

   RO_RETHROWN means the callee always rethrows the in-flight exception, so
   the call is as good as a `throw;' written here.  RO_RETURNED means it
   always returns having done nothing observable, so the caller's walk carries
   on; *VALUE_OUT then holds the returned value where that could be folded.
   RO_FAIL means neither could be shown.

   The recursion is the same predicate applied one frame down, so "did nothing
   else observable first" is enforced at every level for free: a callee that
   logs before rethrowing fails inside the nested walk exactly as it would at
   the top.  */

rethrow_outcome
rethrow_analysis::call_outcome (tree call, aval *value_out)
{
  *value_out = av_unknown ();

  if (m_depth >= RETHROW_MAX_DEPTH)
    return RO_FAIL;

  tree fn = cp_get_callee_fndecl_nofold (call);
  if (!fn || TREE_CODE (fn) != FUNCTION_DECL)
    return RO_FAIL;

  /* A weak definition can be replaced at link time by one that does
     something else.  An inline or COMDAT one cannot: every copy is the same
     definition.  */
  if (DECL_WEAK (fn) && !DECL_DECLARED_INLINE_P (fn) && !DECL_COMDAT (fn))
    return RO_FAIL;

  tree body = DECL_SAVED_TREE (fn);
  if (!body)
    {
      /* A template specialization's definition is only *queued* by the
	 trampoline's use of it, so at this point there is nothing to read.
	 __combined_label::handle_contract_violation is exactly that case, and
	 it is the delegation that matters most, so ask for the definition
	 now.  maybe_instantiate_decl is the guarded entry point -- it raises
	 function_depth first, because instantiating collects and the caller's
	 live trees are only reachable from the stack.  It is a no-op for
	 anything that is not a specialization.  */
      maybe_instantiate_decl (fn);
      body = DECL_SAVED_TREE (fn);
      if (!body)
	return RO_FAIL;
    }

  /* Find the callee parameter, if any, that received our violation object, so
     the accessors keep folding across the delegation.  When none does -- the
     `void helper () { throw; }' shape -- the nested walk simply runs without a
     violation parameter, which is all such a helper needs.  */
  tree nested_parm = NULL_TREE;
  if (m_violation_parm && TREE_CODE (call) == CALL_EXPR)
    {
      tree parm = DECL_ARGUMENTS (fn);
      int nargs = call_expr_nargs (call);
      for (int i = 0; parm && i < nargs; parm = DECL_CHAIN (parm), ++i)
	if (strip_to_decl (CALL_EXPR_ARG (call, i)) == m_violation_parm)
	  {
	    nested_parm = parm;
	    break;
	  }
    }

  rethrow_analysis nested (nested_parm, m_violation_type, m_semantic, m_kind,
			   m_depth + 1);
  rethrow_outcome o = nested.walk_stmt (body);

  switch (o)
    {
    case RO_RETHROWN:
      /* A rethrow out of a noexcept callee terminates rather than
	 propagating, which is not what eliding the catch would do.  */
      if (TYPE_NOTHROW_P (TREE_TYPE (fn)))
	return RO_FAIL;
      return RO_RETHROWN;

    case RO_RETURNED:
      *value_out = nested.returned_value ();
      return RO_RETURNED;

    case RO_FALLTHROUGH:
      /* Ran off the end of a void body: it returned, with no value.  */
      return RO_RETURNED;

    default:
      return RO_FAIL;
    }
}

/* Evaluate T as far as the abstract domain allows.  */

aval
rethrow_analysis::eval (tree t)
{
  if (!t || t == error_mark_node)
    return av_unknown ();

  switch (TREE_CODE (t))
    {
    case INTEGER_CST:
      if (tree_fits_shwi_p (t))
	return av_const (tree_to_shwi (t));
      return av_unknown ();

    case VAR_DECL:
    case PARM_DECL:
      if (aval *v = m_env.get (t))
	return *v;
      if (TREE_THIS_VOLATILE (t))
	m_impure = true;
      return av_unknown ();

    CASE_CONVERT:
    case NON_LVALUE_EXPR:
      /* An integral conversion preserves a tracked value.  Conversions of
	 anything else are transparent only to the extent that what they
	 wrap still evaluates -- the exception_ptr temporary below reaches
	 here.  */
      return eval (TREE_OPERAND (t, 0));

    case CLEANUP_POINT_EXPR:
    case EXPR_STMT:
      return eval (TREE_OPERAND (t, 0));

    case ADDR_EXPR:
      return eval (TREE_OPERAND (t, 0));

    case COMPONENT_REF:
    case INDIRECT_REF:
      /* Reading an object is not observable, unless it is volatile
	 ([intro.abstract]); __combined_label reaches its components through
	 this->_M_first.  */
      if (TREE_THIS_VOLATILE (t))
	{
	  m_impure = true;
	  return av_unknown ();
	}
      eval (TREE_OPERAND (t, 0));
      return av_unknown ();

    case TARGET_EXPR:
      return eval (TARGET_EXPR_INITIAL (t));

    case AGGR_INIT_EXPR:
    case CALL_EXPR:
      {
	aval a;
	if (TREE_CODE (t) == CALL_EXPR && accessor_value (t, &a))
	  return a;
	if (calls_std_fn_p (t, "current_exception"))
	  return av_current_exception ();

	/* The arguments are evaluated first, the object of a member call
	   among them; a side effect there is observable whatever the callee
	   does.  */
	if (TREE_CODE (t) == CALL_EXPR)
	  for (int i = 0; i < call_expr_nargs (t); ++i)
	    eval (CALL_EXPR_ARG (t, i));
	else
	  for (int i = 0; i < aggr_init_expr_nargs (t); ++i)
	    eval (AGGR_INIT_EXPR_ARG (t, i));
	if (m_impure)
	  return av_unknown ();

	/* Otherwise follow the callee.  It may return something knowable
	   having done nothing observable, in which case the value stands in
	   for the call; or it may always rethrow, in which case the
	   expression yields no value and the statement containing it has to
	   be told.  */
	aval v;
	switch (call_outcome (t, &v))
	  {
	  case RO_RETURNED:
	    return v;
	  case RO_RETHROWN:
	    m_rethrew = true;
	    m_impure = true;
	    return av_unknown ();
	  default:
	    m_impure = true;
	    return av_unknown ();
	  }
      }

    case EQ_EXPR:
    case NE_EXPR:
    case LT_EXPR:
    case LE_EXPR:
    case GT_EXPR:
    case GE_EXPR:
      {
	aval l = eval (TREE_OPERAND (t, 0));
	aval r = eval (TREE_OPERAND (t, 1));
	if (l.kind != AV_CONST || r.kind != AV_CONST)
	  return av_unknown ();
	bool res;
	switch (TREE_CODE (t))
	  {
	  case EQ_EXPR: res = (l.val == r.val); break;
	  case NE_EXPR: res = (l.val != r.val); break;
	  case LT_EXPR: res = (l.val < r.val); break;
	  case LE_EXPR: res = (l.val <= r.val); break;
	  case GT_EXPR: res = (l.val > r.val); break;
	  default:	res = (l.val >= r.val); break;
	  }
	return av_const (res);
      }

    case TRUTH_NOT_EXPR:
      {
	aval a = eval (TREE_OPERAND (t, 0));
	if (a.kind != AV_CONST)
	  return av_unknown ();
	return av_const (!a.val);
      }

    case TRUTH_AND_EXPR:
    case TRUTH_ANDIF_EXPR:
      {
	aval l = eval (TREE_OPERAND (t, 0));
	if (l.kind == AV_CONST && !l.val)
	  return av_const (0);
	aval r = eval (TREE_OPERAND (t, 1));
	if (l.kind != AV_CONST || r.kind != AV_CONST)
	  return av_unknown ();
	return av_const (l.val && r.val);
      }

    case TRUTH_OR_EXPR:
    case TRUTH_ORIF_EXPR:
      {
	aval l = eval (TREE_OPERAND (t, 0));
	if (l.kind == AV_CONST && l.val)
	  return av_const (1);
	aval r = eval (TREE_OPERAND (t, 1));
	if (l.kind != AV_CONST || r.kind != AV_CONST)
	  return av_unknown ();
	return av_const (l.val || r.val);
      }

    default:
      m_impure = true;
      return av_unknown ();
    }
}

/* Walk statement T under the assumption that the violation was detected as
   CDM_EVAL_EXCEPTION, reporting how control leaves it.  */

rethrow_outcome
rethrow_analysis::walk_stmt (tree t)
{
  if (!t)
    return RO_FALLTHROUGH;
  if (t == error_mark_node)
    return RO_FAIL;

  switch (TREE_CODE (t))
    {
    case STATEMENT_LIST:
      for (tree_stmt_iterator i = tsi_start (t); !tsi_end_p (i); tsi_next (&i))
	{
	  rethrow_outcome o = walk_stmt (tsi_stmt (i));
	  if (o != RO_FALLTHROUGH)
	    return o;
	}
      return RO_FALLTHROUGH;

    case BIND_EXPR:
      return walk_stmt (BIND_EXPR_BODY (t));

    case CLEANUP_POINT_EXPR:
    case EXPR_STMT:
      return walk_stmt (TREE_OPERAND (t, 0));

    case MUST_NOT_THROW_EXPR:
      /* A region an exception may not leave -- a noexcept function's body,
	 among others.  A rethrow inside it terminates rather than
	 propagating, so it must not count as reaching the caller; but code
	 that merely runs and returns is unremarkable and the walk carries
	 on.  Bailing outright instead would lose the optimization for a
	 handler that calls any nothrow function, however trivial, before
	 rethrowing.  */
      {
	rethrow_outcome o = walk_stmt (TREE_OPERAND (t, 0));
	return o == RO_RETHROWN ? RO_FAIL : o;
      }

    case DEBUG_BEGIN_STMT:
      /* A statement-frontier marker, emitted throughout every statement list
	 under -g.  It carries no code, and must not reach the default below,
	 which would switch the whole analysis off in any build with -g.  */
      return RO_FALLTHROUGH;

    CASE_CONVERT:
    case NON_LVALUE_EXPR:
      /* A discarded-value conversion.  With no side effects there is nothing
	 to model -- an empty else-arm arrives here as a void NOP_EXPR of
	 integer zero.  */
      if (!TREE_SIDE_EFFECTS (t))
	return RO_FALLTHROUGH;
      return walk_stmt (TREE_OPERAND (t, 0));

    case DECL_EXPR:
      {
	tree decl = DECL_EXPR_DECL (t);
	if (!decl)
	  return RO_FAIL;
	if (TREE_CODE (decl) == TYPE_DECL || TREE_CODE (decl) == USING_DECL)
	  return RO_FALLTHROUGH;
	if (!VAR_P (decl) || TREE_STATIC (decl) || DECL_EXTERNAL (decl))
	  return RO_FAIL;
	/* Only scalars: a class-typed local brings a destructor, and with it
	   cleanup control flow this walk does not model.  */
	if (!SCALAR_TYPE_P (TREE_TYPE (decl)))
	  return RO_FAIL;

	/* An initializer that is a call has to be asked whether it returns at
	   all before it is asked what it produces.  */
	aval init = av_unknown ();
	rethrow_outcome o = eval_value (DECL_INITIAL (decl), &init);
	if (o != RO_FALLTHROUGH)
	  return o;

	m_env.put (decl, init);
	return RO_FALLTHROUGH;
      }

    case MODIFY_EXPR:
    case INIT_EXPR:
      {
	tree lhs = TREE_OPERAND (t, 0);
	/* Only assignments to locals we are already tracking; a store
	   anywhere else is an observable effect.  */
	if (!VAR_P (lhs) || !m_env.get (lhs))
	  return RO_FAIL;

	/* Same as above: a call on the right may never produce a value at
	   all.  This is the shape __combined_label delegation takes --
	   `__r = _M_lhs.handle_contract_violation (__v)'.  */
	aval rhs = av_unknown ();
	rethrow_outcome o = eval_value (TREE_OPERAND (t, 1), &rhs);
	if (o != RO_FALLTHROUGH)
	  return o;

	m_env.put (lhs, rhs);
	return RO_FALLTHROUGH;
      }

    case COND_EXPR:
      {
	aval c = av_unknown ();
	rethrow_outcome o = eval_value (TREE_OPERAND (t, 0), &c);
	if (o != RO_FALLTHROUGH)
	  return o;
	if (c.kind != AV_CONST)
	  return RO_FAIL;
	return walk_stmt (TREE_OPERAND (t, c.val ? 1 : 2));
      }

    case SWITCH_EXPR:
      return walk_switch (t);

    case GOTO_EXPR:
      /* The genericized form of a break from the switch being walked.  */
      if (m_break_label && GOTO_DESTINATION (t) == m_break_label)
	return RO_BREAK;
      return RO_FAIL;

    case THROW_EXPR:
      /* `throw;' is a call to __cxa_rethrow.  `throw X;' raises a different
	 exception, which is not what eliding the catch would do.  */
      {
	tree op = TREE_OPERAND (t, 0);
	tree fn = op ? cp_get_callee_fndecl_nofold (op) : NULL_TREE;
	if (fn && DECL_NAME (fn) && id_equal (DECL_NAME (fn), "__cxa_rethrow"))
	  return RO_RETHROWN;
	return RO_FAIL;
      }

    case CALL_EXPR:
      /* std::rethrow_exception (std::current_exception ()) rethrows the
	 exception that is in flight, so it reaches the same place.  */
      if (calls_std_fn_p (t, "rethrow_exception")
	  && call_expr_nargs (t) == 1
	  && eval (CALL_EXPR_ARG (t, 0)).kind == AV_CURRENT_EXCEPTION)
	return RO_RETHROWN;
      /* A discarded call to one of the folded accessors does nothing.  */
      {
	aval a;
	if (accessor_value (t, &a))
	  return RO_FALLTHROUGH;
      }
      /* Otherwise let eval follow the callee: it may itself always rethrow,
	 or return having done nothing, in which case the walk continues
	 here with the value discarded.  */
      {
	aval discarded = av_unknown ();
	return eval_value (t, &discarded);
      }

    case RETURN_EXPR:
      /* Record what was returned, for a caller that is following this call.
	 In GENERIC the operand is an assignment to the RESULT_DECL.  */
      {
	tree op = TREE_OPERAND (t, 0);
	tree val = op;
	if (op
	    && (TREE_CODE (op) == MODIFY_EXPR || TREE_CODE (op) == INIT_EXPR))
	  val = TREE_OPERAND (op, 1);

	/* `return helper ();' has to follow the callee like any other call --
	   it may rethrow, and it certainly may do something.  */
	aval v = av_unknown ();
	rethrow_outcome o = eval_value (val, &v);
	if (o != RO_FALLTHROUGH)
	  return o;

	m_returned = v;
	return RO_RETURNED;
      }

    default:
      return RO_FAIL;
    }
}

/* Append the statements of T to OUT, descending into statement lists and
   blocks: the flat sequence a switch's case labels sit in.  */

static void
flatten_switch_body (tree t, vec<tree> *out)
{
  if (!t)
    return;
  if (TREE_CODE (t) == STATEMENT_LIST)
    for (tree_stmt_iterator i = tsi_start (t); !tsi_end_p (i); tsi_next (&i))
      flatten_switch_body (tsi_stmt (i), out);
  else if (TREE_CODE (t) == BIND_EXPR)
    flatten_switch_body (BIND_EXPR_BODY (t), out);
  else
    out->safe_push (t);
}

/* walk_tree callback: find a case label that is not one of the switch's own,
   that is, one nested inside a statement, skipping inner switches.  */

static tree
find_nested_case_label (tree *tp, int *walk_subtrees, void *)
{
  if (TREE_CODE (*tp) == SWITCH_EXPR)
    *walk_subtrees = 0;
  else if (TREE_CODE (*tp) == CASE_LABEL_EXPR)
    return *tp;
  return NULL_TREE;
}

/* Walk T, a SWITCH_EXPR: fold its condition, find the case label it selects
   (or the default), and walk on from there through the labels that follow,
   which control falls through, until a break leaves the switch.  No matching
   label and no default skips the body.  Anything irregular -- a condition
   the domain cannot fold, a case label nested inside a statement, a jump
   other than a break -- fails.  */

rethrow_outcome
rethrow_analysis::walk_switch (tree t)
{
  aval c = av_unknown ();
  rethrow_outcome o = eval_value (SWITCH_COND (t), &c);
  if (o != RO_FALLTHROUGH)
    return o;
  if (c.kind != AV_CONST)
    return RO_FAIL;

  auto_vec<tree, 16> body;
  flatten_switch_body (SWITCH_BODY (t), &body);

  /* The case labels must all be at the top level, and the break label, if
     a break uses it, is the body's last statement.  */
  tree break_label = NULL_TREE;
  int start = -1, dflt = -1;
  for (unsigned i = 0; i < body.length (); ++i)
    {
      tree stmt = body[i];
      if (TREE_CODE (stmt) == CASE_LABEL_EXPR)
	{
	  tree low = CASE_LOW (stmt), high = CASE_HIGH (stmt);
	  if (!low)
	    dflt = i;
	  else if (start < 0)
	    {
	      if (!tree_fits_shwi_p (low)
		  || (high && !tree_fits_shwi_p (high)))
		return RO_FAIL;
	      HOST_WIDE_INT lo = tree_to_shwi (low);
	      HOST_WIDE_INT hi = high ? tree_to_shwi (high) : lo;
	      if (lo <= c.val && c.val <= hi)
		start = i;
	    }
	}
      else if (TREE_CODE (stmt) == LABEL_EXPR
	       && i == body.length () - 1
	       && LABEL_DECL_BREAK (LABEL_EXPR_LABEL (stmt)))
	break_label = LABEL_EXPR_LABEL (stmt);
      else if (walk_tree_without_duplicates (&body[i], find_nested_case_label,
					     NULL))
	return RO_FAIL;
    }
  if (start < 0)
    start = dflt;
  if (start < 0)
    return RO_FALLTHROUGH;

  tree saved_break_label = m_break_label;
  m_break_label = break_label;
  o = RO_FALLTHROUGH;
  for (unsigned i = start; i < body.length () && o == RO_FALLTHROUGH; ++i)
    {
      tree stmt = body[i];
      if (TREE_CODE (stmt) == CASE_LABEL_EXPR
	  || (TREE_CODE (stmt) == LABEL_EXPR
	      && LABEL_EXPR_LABEL (stmt) == break_label))
	continue;
      o = walk_stmt (stmt);
    }
  m_break_label = saved_break_label;
  return o == RO_BREAK ? RO_FALLTHROUGH : o;
}

} // anon namespace

/* Return true if CONTRACT's label carries a local violation handler that,
   for a violation detected as CDM_EVAL_EXCEPTION under SEMANTIC and KIND,
   always exits by rethrowing the in-flight exception without first doing
   anything else observable.  When it does, the caller may skip wrapping the
   predicate in a try/catch: the exception reaches the same place either way.

   Conservative -- false whenever this cannot be proven.  */

static bool
contract_local_handler_always_rethrows_p (tree contract,
					  contract_evaluation_semantic semantic,
					  contract_assertion_kind kind)
{
  /* Only the two semantics whose handler may legitimately let an exception
     escape.  */
  if (semantic != CES_ENFORCE && semantic != CES_OBSERVE)
    return false;

  /* Mirror the conditions under which build_contract_data_block_ctor actually
     records a local handler; without one there is nothing to reason about.  */
  tree label = CONTRACT_LABEL (contract);
  if (!label || label == error_mark_node || !VAR_P (label))
    return false;
  tree label_type = TREE_TYPE (label);
  if (!label_type || !TYPE_P (label_type))
    return false;
  label_type = TYPE_MAIN_VARIANT (label_type);

  if (!local_violation_trampoline_map
      || !local_violation_trampoline_map->get (label_type)
      || !local_violation_handler_fn_map)
    return false;

  tree *fnp = local_violation_handler_fn_map->get (label_type);
  if (!fnp)
    return false;
  tree fn = *fnp;

  /* A noexcept handler cannot rethrow -- it would terminate.  */
  if (TYPE_NOTHROW_P (TREE_TYPE (fn)))
    return false;

  /* The body has to be here to be read.  It is absent for a handler defined
     out of line later in the translation unit, and for a template member
     whose instantiation the trampoline's use has only queued -- which is the
     case for __combined_label's handler, so ask for that one now.  */
  tree body = DECL_SAVED_TREE (fn);
  if (!body)
    {
      maybe_instantiate_decl (fn);
      body = DECL_SAVED_TREE (fn);
      if (!body)
	return false;
    }

  /* The violation parameter is the last one; for a non-static member
     function DECL_ARGUMENTS starts with `this'.  */
  tree parm = DECL_ARGUMENTS (fn);
  if (!parm)
    return false;
  while (DECL_CHAIN (parm))
    parm = DECL_CHAIN (parm);

  tree violation_type
    = lookup_std_contracts_type (get_identifier ("contract_violation"));
  if (!violation_type || violation_type == error_mark_node
      || !TYPE_P (violation_type))
    return false;

  rethrow_analysis analysis (parm, TYPE_MAIN_VARIANT (violation_type),
			     semantic, kind);
  return analysis.walk_stmt (body) == RO_RETHROWN;
}

/* The evaluation of CONTRACT with the P3100 assume semantic: an optimizer
   assumption of its predicate, lowered as [[assume]] lowers its own
   (IFN_ASSUME), so the predicate is never evaluated -- one with side effects
   is outlined into a function that is never called -- and the optimizers
   may take it to hold where the evaluation happens.  void_node for a
   postcondition with captures, which are not initialized
   (contract_lowered_as_assumption_p); NULL_TREE on error.  */

static tree
build_contract_assumption (tree contract)
{
  if (!contract_lowered_as_assumption_p (contract, CES_ASSUME))
    return void_node;
  remap_dummy_this (current_function_decl, &CONTRACT_CONDITION (contract));
  if (CONTRACT_CONDITION (contract) == error_mark_node)
    return NULL_TREE;
  if (POSTCONDITION_P (contract) && !flag_contract_checks_outlined)
    {
      remap_retval (current_function_decl, contract);
      if (CONTRACT_CONDITION (contract) == error_mark_node)
	return NULL_TREE;
    }
  tree cond = unshare_expr (CONTRACT_CONDITION (contract));
  cond = fold_build_cleanup_point_expr (TREE_TYPE (cond), cond);
  return build_call_expr_internal_loc (EXPR_LOCATION (contract), IFN_ASSUME,
				       void_type_node, 1, cond);
}

/* D4405: true if ANALYSIS, the semantic of CONTRACT's analysis
   evaluation, is prove or diagnose and the current function makes that
   evaluation (build_analysis_evaluation).  */

static bool
contract_analyzed_here_p (tree contract,
			  contract_evaluation_semantic analysis)
{
  return (contract_semantic_proving_p (analysis)
	  && !PRECONDITION_P (contract)
	  && !DECL_IS_WRAPPER_FN_P (current_function_decl));
}

/* D4405: the analysis evaluation of CONTRACT, a postcondition or contract
   assertion, with the semantic ANALYSIS, in the function body ahead of the
   assertion's runtime evaluation there, so that the proof never takes that
   check as a premise: if ANALYSIS is prove or diagnose, the evaluation is
   diagnosed unless proven -- or a strategy's marker, which this returns,
   takes its place.  NULL_TREE if there is nothing to emit.  The predicate
   is never evaluated.  A precondition's analysis evaluation is at each
   call, in the caller (maybe_contract_wrap_call), and a contract wrapper
   evaluates only the runtime evaluations it is for.  */

static tree
build_analysis_evaluation (tree contract,
			   contract_evaluation_semantic analysis)
{
  if (!contract_analyzed_here_p (contract, analysis)
      || CONTRACT_CONDITION (contract) == error_mark_node)
    return NULL_TREE;
  /* P4405 proving hook: a strategy's marker takes the evaluation's place if
     it defers the evaluation.  */
  tree marker = NULL_TREE;
  tree pred = (contract_proof_deferrable_p (contract)
	       ? contract_check_condition (contract) : NULL_TREE);
  diagnose_unproven_contract (contract, analysis, EXPR_LOCATION (contract),
			      NULL_TREE, NULL_TREE, pred ? &marker : NULL,
			      pred);
  return marker;
}

/* Emit the check body for CONTRACT under a single, statically known
   evaluation SEMANTIC.  Returns a BIND_EXPR statement expression, or
   void_node when the semantic emits no check (ignore), what assume emits
   instead, or NULL_TREE on error.  This
   is the per-semantic core shared by the plain (compile-time-resolved) path
   and the P3595 dynamic-dispatch path.

   SHARED_DATA_ADDR, when non-NULL, is the address of a violation data block
   already built for this contract; the check reuses it instead of building a
   fresh block.  The P3595 dynamic path passes a single block shared across all
   dispatch arms (the block is identical for every arm of one contract), so we
   emit one global instead of one per arm.  When NULL (the plain path) the
   handler-calling semantics build their own block, as before.  */

static tree
emit_check_for_semantic (tree contract, contract_evaluation_semantic semantic,
			 tree shared_data_addr = NULL_TREE)
{
  bool quick = false;
  bool calls_handler = false;
  switch (semantic)
    {
    case CES_IGNORE:
    case CES_ASSUME:
      /* P3100: assume, which never evaluates the predicate either, is an
	 optimizer assumption.  */
      if (semantic == CES_ASSUME)
	return build_contract_assumption (contract);
      return void_node;
    case CES_PROVE:
    case CES_DIAGNOSE:
      /* D4405: only the analysis evaluation is ever prove or diagnose
	 (build_analysis_evaluation); a runtime evaluation never is.  */
      gcc_checking_assert (false);
      return void_node;
    case CES_ENFORCE:
    case CES_OBSERVE:
    case CES_NOEXCEPT_ENFORCE:
    case CES_NOEXCEPT_OBSERVE:
      calls_handler = true;
      break;
    case CES_QUICK:
      quick = true;
      break;
    default:
      gcc_unreachable ();
    }

  location_t loc = EXPR_LOCATION (contract);

  remap_dummy_this (current_function_decl, &CONTRACT_CONDITION (contract));
  if (CONTRACT_CONDITION (contract) == error_mark_node)
    return NULL_TREE;

  if (POSTCONDITION_P (contract) && !flag_contract_checks_outlined)
    {
      remap_retval (current_function_decl, contract);
      if (CONTRACT_CONDITION (contract) == error_mark_node)
	return NULL_TREE;
    }

  /* Unshare the condition: this helper may be called several times for the
     same contract (once per case of the P3595 dynamic-dispatch switch), so
     each emitted check must own an independent copy of the condition tree.
     The remap steps above operate idempotently on the shared slot.  */
  tree condition = unshare_expr (CONTRACT_CONDITION (contract));

  /* Determine the assertion kind for entry point selection.  */
  contract_assertion_kind kind = get_contract_assertion_kind (contract);

  bool check_might_throw = (flag_exceptions
			    && !expr_noexcept_p (condition, tf_none));
  /* If the label's local violation handler answers an evaluation_exception by
     rethrowing, catching the predicate's exception only to hand it to that
     handler is pure overhead -- let it propagate instead.  */
  if (check_might_throw
      && flag_contract_bypass_rethrowing_local_handler
      && contract_local_handler_always_rethrows_p (contract, semantic, kind))
    check_might_throw = false;

  /* Build a statement expression to hold a contract check, with the check
     potentially wrapped in a try-catch expr.  */
  tree cc_bind = build3 (BIND_EXPR, void_type_node, NULL, NULL, NULL);
  BIND_EXPR_BODY (cc_bind) = push_stmt_list ();

  if (TREE_CODE (contract) == ASSERTION_STMT)
    emit_builtin_observable_checkpoint ();
  tree cond = build_x_unary_op (loc, TRUTH_NOT_EXPR, condition, NULL_TREE,
				tf_warning_or_error);

  tree data_addr = NULL_TREE;
  if (calls_handler)
    {
      if (shared_data_addr)
	/* Reuse the block built once for all dynamic-dispatch arms.  */
	data_addr = shared_data_addr;
      else
	{
	  /* Build a data block for the violation.  */
	  data_addr = build_contract_data_block_address (contract);
	}
    }

  /* Get the entry points we will call.  */
  tree entry_pf = NULL_TREE;
  tree entry_ex = NULL_TREE;
  if (calls_handler)
    {
      entry_pf = declare_cxa_entry_point (kind, semantic,
					  CDM_PREDICATE_FALSE);
      if (check_might_throw)
	entry_ex = declare_cxa_entry_point (kind, semantic,
					    CDM_EVAL_EXCEPTION);
    }

  if (check_might_throw)
    {
      tree check_failed = build_decl (loc, VAR_DECL, NULL, boolean_type_node);
      DECL_ARTIFICIAL (check_failed) = true;
      DECL_IGNORED_P (check_failed) = true;
      DECL_CONTEXT (check_failed) = current_function_decl;
      layout_decl (check_failed, 0);
      add_decl_expr (check_failed);
      DECL_CHAIN (check_failed) = BIND_EXPR_VARS (cc_bind);
      BIND_EXPR_VARS (cc_bind) = check_failed;
      tree check_try = begin_try_block ();
      finish_expr_stmt (cp_build_init_expr (check_failed, cond));
      finish_try_block (check_try);

      tree handler = begin_handler ();
      finish_handler_parms (NULL_TREE, handler); /* catch (...) */
      if (quick)
	finish_expr_stmt (build_quick_enforce_reaction (loc));
      else
	/* Call the _ex variant with the same data block.
	   Detection mode (evaluation_exception) is encoded in the entry
	   point name, not the data.  */
	finish_expr_stmt (build_call_n (entry_ex, 1, data_addr));
      /* The exception has been handled; the check below must not fire too.
	 The quick_enforce reaction does not return.  */
      if (!quick)
	finish_expr_stmt (cp_build_modify_expr (loc, check_failed, NOP_EXPR,
						boolean_false_node,
						tf_warning_or_error));
      finish_handler (handler);
      finish_handler_sequence (check_try);
      cond = check_failed;
      BIND_EXPR_VARS (cc_bind) = nreverse (BIND_EXPR_VARS (cc_bind));
    }

  tree do_check = begin_if_stmt ();
  finish_if_stmt_cond (cond, do_check);
  /* The violation arm is unlikely, as [[unlikely]] would mark it: whatever
     the semantic, the success branch is the likely one.  */
  add_stmt (build_predict_expr (PRED_COLD_LABEL, NOT_TAKEN));
  if (quick)
    finish_expr_stmt (build_quick_enforce_reaction (loc));
  else
    {
      finish_expr_stmt (build_call_n (entry_pf, 1, data_addr));
      if (semantic == CES_OBSERVE)
	emit_builtin_observable_checkpoint ();
    }
  finish_then_clause (do_check);
  finish_if_stmt (do_check);

  TREE_SIDE_EFFECTS (cc_bind) = true;
  BIND_EXPR_BODY (cc_bind) = pop_stmt_list (BIND_EXPR_BODY (cc_bind));
  return cc_bind;
}

/* Emit an unconditional enforced violation for CONTRACT: report the
   violation as if a predicate evaluated with SEMANTIC, enforce or
   noexcept_enforce, had evaluated false, then terminate; or, when SEMANTIC
   is quick_enforce, trap inline without reporting it.  Used as the
   default arm of the P3595 dynamic-dispatch switch, for a selector value
   that is unknown or maps to no valid semantic (P3595).  Returns a
   BIND_EXPR statement expression (the trap for quick_enforce), or
   NULL_TREE on error.  SHARED_DATA_ADDR
   is the address of the violation data block already built once for this
   contract's dynamic dispatch; it is reused here (the block is identical to
   the one every dispatch arm uses).  */

static tree
emit_enforced_violation (tree contract, tree shared_data_addr,
			 contract_evaluation_semantic semantic)
{
  if (semantic == CES_QUICK)
    return build_quick_enforce_reaction (EXPR_LOCATION (contract));

  contract_assertion_kind kind = get_contract_assertion_kind (contract);

  tree cc_bind = build3 (BIND_EXPR, void_type_node, NULL, NULL, NULL);
  BIND_EXPR_BODY (cc_bind) = push_stmt_list ();

  /* Reuse the violation data block built once for the dynamic dispatch.  */

  /* Call the predicate-false entry point unconditionally.  That entry
     point is noreturn for both semantics, so no explicit terminate is
     required after it.  */
  tree entry_pf = declare_cxa_entry_point (kind, semantic,
					   CDM_PREDICATE_FALSE);
  finish_expr_stmt (build_call_n (entry_pf, 1, shared_data_addr));

  BIND_EXPR_BODY (cc_bind) = pop_stmt_list (BIND_EXPR_BODY (cc_bind));
  return cc_bind;
}

/* P3100: build the violation data block for an implicit contract assertion
   at LOC with comment COMMENT, and return its address.  The assertion is
   synthesized for the purpose; it has no label, so the block is the basic
   one.  Builds GENERIC.  */

static tree
build_implicit_violation_data (location_t loc, const char *comment)
{
  tree contract = make_node (ASSERTION_STMT);
  TREE_TYPE (contract) = void_type_node;
  SET_EXPR_LOCATION (contract, loc);
  CONTRACT_COMMENT (contract) = build_string_literal (comment);
  return build_contract_data_block_address (contract);
}

/* P3100: build the call reporting a violation of an implicit contract
   assertion at LOC with comment COMMENT to the CAK_IMPLICIT entry point for
   SEM, one of the four semantics that call the handler.  */

static tree
build_implicit_violation_call (location_t loc,
			       contract_evaluation_semantic sem,
			       const char *comment)
{
  tree entry = declare_cxa_entry_point (CAK_IMPLICIT, sem,
					CDM_PREDICATE_FALSE);
  tree call = build_call_n (entry, 1,
			    build_implicit_violation_data (loc, comment));
  SET_EXPR_LOCATION (call, loc);
  return call;
}

/* P3100: build the GENERIC code to append at the fall-off point of a
   value-returning function whose implicit {stmt.return.flow.off} assertion
   resolved to SEM.  FNDECL is the function and LOC the site.  Returns a
   statement (or STATEMENT_LIST) to append to the function body, or NULL_TREE
   when the caller should keep the legacy behaviour (SEM == CES_ASSUME) or when
   no code is needed.

   Reaching the end of a value-returning function is always a violation, so the
   emitted code is an unconditional reaction, not a guarded check:

     ignore                   -> return a defined (erroneous) value; no handler
     quick_enforce            -> call the terminate handler (noreturn)
     enforce/noexcept_enforce -> call the noreturn violation entry point
     observe/noexcept_observe -> call the (returning) violation entry point,
				 then return a defined (erroneous) value

   The "defined value" zeroes the bytes of the result object regardless of the
   return type: a zero value for a scalar, and a memset of the whole object
   (including padding) for a class or array, so no indeterminate data is leaked.

   The violation is reported through the CAK_IMPLICIT entry points, so a handler
   observes assertion_kind::implicit (P3100).  This helper runs after
   genericization, so it must build GENERIC (not front-end statement) trees.  */

tree
build_implicit_flow_off_check (tree fndecl, location_t loc,
			       contract_evaluation_semantic sem)
{
  if (sem == CES_ASSUME)
    return NULL_TREE;

  /* A defined (erroneous) return that zeroes the bytes of the result object, so
     no indeterminate data is leaked regardless of the return type.  For a
     scalar this is a zero value; for a class/array the whole object storage is
     cleared with memset (every byte, including padding) -- well-defined for a
     trivially-constructible type, and never a data leak otherwise.  */
  tree defined_return = NULL_TREE;
  tree res = DECL_RESULT (fndecl);
  if (res)
    {
      tree obj = DECL_BY_REFERENCE (res) ? build_fold_indirect_ref (res) : res;
      tree objtype = TREE_TYPE (obj);
      if (SCALAR_TYPE_P (objtype))
	{
	  tree modify = build2 (MODIFY_EXPR, objtype, obj,
				build_zero_cst (objtype));
	  defined_return = build1 (RETURN_EXPR, void_type_node, modify);
	}
      else
	{
	  tree memset_call
	    = build_call_expr (builtin_decl_explicit (BUILT_IN_MEMSET), 3,
			       build_fold_addr_expr (obj), integer_zero_node,
			       fold_convert (size_type_node,
					     TYPE_SIZE_UNIT (objtype)));
	  tree ret = build1 (RETURN_EXPR, void_type_node, res);
	  defined_return = build2 (COMPOUND_EXPR, void_type_node,
				   memset_call, ret);
	}
      SET_EXPR_LOCATION (defined_return, loc);
    }

  if (sem == CES_IGNORE)
    return defined_return;   /* Zeroed result of any type (NULL only if no
			       result decl).  */

  if (sem == CES_QUICK)
    return build_quick_enforce_reaction (loc);

  /* enforce / observe / noexcept_enforce / noexcept_observe: call the
     corresponding CAK_IMPLICIT entry point.  */
  tree call = build_implicit_violation_call (loc, sem,
					     "control reached the end of a "
					     "value-returning function");

  if (sem == CES_ENFORCE || sem == CES_NOEXCEPT_ENFORCE)
    /* The entry point is noreturn; nothing follows.  */
    return call;

  /* observe / noexcept_observe: the handler returns, then continue with a
     defined return value.  */
  tree list = NULL_TREE;
  append_to_statement_list (call, &list);
  if (defined_return)
    append_to_statement_list (defined_return, &list);
  return list;
}

/* P3100: build the reaction for control flowing off the end of a coroutine
   whose promise type has no usable return_void
   ({stmt.return.coroutine.flow.off}), for the resolved semantic SEM.  This is
   the coroutine analogue of build_implicit_flow_off_check, but there is no
   return value to substitute: the coroutine's return object was created at
   get_return_object, so the continuing semantics simply proceed to the final
   suspend.  Returns

     assume/ignore            -> NULL_TREE (fall through to final
				 suspend, as today);
     quick_enforce            -> call the terminate handler (noreturn);
     enforce/noexcept_enforce -> call the noreturn violation entry point;
     observe/noexcept_observe -> call the (returning) violation entry
				 point, then continue.

   The reaction is emitted inside the coroutine's try block, so a throwing
   enforce/observe is caught by promise.unhandled_exception ().  Reported
   through the CAK_IMPLICIT entry points (assertion_kind::implicit).  Builds
   GENERIC.  */

bool
cp_build_implicit_ub_handler (tree fndecl, location_t loc, const char *group,
			      int reaction, tree *entry_out,
			      tree *data_addr_out)
{
  implicit_ub_info info;
  if (!flag_contracts_p3100 || fndecl == NULL_TREE
      || !implicit_ub_group_info (group, &info))
    return false;

  /* Use the reaction resolved once, pre-inline, rather than re-resolving here
     against the (possibly inlined-into) FNDECL.  */
  contract_evaluation_semantic sem;
  if (reaction == IMPLICIT_UB_NOEXCEPT_ENFORCE)
    sem = CES_NOEXCEPT_ENFORCE;
  else if (reaction == IMPLICIT_UB_NOEXCEPT_OBSERVE)
    sem = CES_NOEXCEPT_OBSERVE;
  else
    return false;

  *data_addr_out = build_implicit_violation_data (loc, info.comment);
  *entry_out = declare_cxa_entry_point (CAK_IMPLICIT, sem,
					CDM_PREDICATE_FALSE);
  return true;
}

/* P3100: shared helper to guard a scalar operation OP_RESULT that is UB when
   COND holds, for the resolved semantic SEM (never CES_ASSUME).  Returns

     force, (cond ? <reaction-value> : op_result)

   FORCE is evaluated first so an operand referenced only on the ok-path (e.g. a
   division's dividend) still has its side effects on the violation path; all
   operands are SAVE_EXPRs by the time we get here, so each is evaluated once.
   On the violation path the UB operation is not executed:

     ignore              -> a defined (erroneous) zero;
     observe/nx_observe  -> call the handler, then a defined zero;
     quick_enforce       -> terminate;
     enforce/nx_enforce  -> call the noreturn handler.

   COMMENT is the contract-violation comment recorded for the check.  SUBST,
   if given, replaces the defined zero: an operation with a defined fallback
   other than zero (pointer arithmetic leaving the pointer unmoved).  */

static tree
build_implicit_op_guard (location_t loc, contract_evaluation_semantic sem,
			 tree force, tree cond, tree op_result,
			 const char *comment, tree subst = NULL_TREE)
{
  tree restype = TREE_TYPE (op_result);
  tree zero = subst ? subst : build_zero_cst (restype);

  tree viol_value;
  if (sem == CES_IGNORE)
    viol_value = zero;
  else if (sem == CES_QUICK)
    {
      tree call = build_quick_enforce_reaction (loc);
      viol_value = build2 (COMPOUND_EXPR, restype, call, zero);
    }
  else
    {
      /* enforce / observe / noexcept_enforce / noexcept_observe: build a
	 violation data block and call the corresponding CAK_IMPLICIT entry
	 point.  The enforce entry is noreturn; observe returns and we continue
	 with the defined zero.  In value position both are paired with the
	 defined zero via COMPOUND_EXPR (dead after a noreturn enforce call, but
	 needed for the type).  */
      tree call = build_implicit_violation_call (loc, sem, comment);
      viol_value = build2 (COMPOUND_EXPR, restype, call, zero);
    }

  tree guarded = build3_loc (loc, COND_EXPR, restype, cond, viol_value,
			     op_result);
  COND_EXPR_IMPLICIT_UB_GUARD_P (guarded) = true;
  guarded = build2 (COMPOUND_EXPR, restype, force, guarded);
  return guarded;
}

/* P3100: guard an integer division/remainder DIV_RESULT (dividend OP0, divisor
   OP1) whose divisor may be zero -- core-language UB ({expr.mul.div.by.zero}).
   Returns `op0, (op1 == 0 ? <reaction> : div_result)`.  */

tree
build_implicit_divide_check (tree ARG_UNUSED (fndecl), location_t loc,
			     contract_evaluation_semantic sem,
			     tree op0, tree op1, tree div_result)
{
  gcc_checking_assert (sem != CES_ASSUME);

  tree cond = build2_loc (loc, EQ_EXPR, boolean_type_node,
			  op1, build_zero_cst (TREE_TYPE (op1)));
  return build_implicit_op_guard (loc, sem, op0, cond, div_result,
				  "integer division by zero");
}

/* P3100: guard a floating-point division DIV_RESULT (dividend OP0, divisor
   OP1) whose divisor may be zero -- core-language UB ({expr.mul.div.by.zero}),
   though IEEE arithmetic would yield an infinity or a NaN.  The comparison
   with zero holds for -0.0 as well, and never for a NaN.  Returns
   `op0, (op1 == 0 ? <reaction / 0> : div_result)`.  */

tree
build_implicit_float_divide_check (tree ARG_UNUSED (fndecl), location_t loc,
				   contract_evaluation_semantic sem,
				   tree op0, tree op1, tree div_result)
{
  gcc_checking_assert (sem != CES_ASSUME);

  tree cond = build2_loc (loc, EQ_EXPR, boolean_type_node,
			  op1, build_real (TREE_TYPE (op1), dconst0));
  return build_implicit_op_guard (loc, sem, op0, cond, div_result,
				  "floating-point division by zero");
}

/* P3100: guard a signed integer division/remainder DIV_RESULT (OP0 / OP1) whose
   quotient is not representable -- i.e. OP0 == the type minimum and
   OP1 == -1 -- core-language UB ({expr.mul.representable}).  Returns
   `op0, (op0 == MIN && op1 == -1 ? <reaction> : div_result)`.  */

tree
build_implicit_divide_overflow_check (tree ARG_UNUSED (fndecl), location_t loc,
				      contract_evaluation_semantic sem,
				      tree op0, tree op1, tree div_result)
{
  gcc_checking_assert (sem != CES_ASSUME);

  tree type = TREE_TYPE (op0);
  tree min_val = build2_loc (loc, EQ_EXPR, boolean_type_node, op0,
			     TYPE_MIN_VALUE (type));
  tree neg_one = build2_loc (loc, EQ_EXPR, boolean_type_node, op1,
			     build_int_cst (TREE_TYPE (op1), -1));
  /* Use non-short-circuiting AND so both OP0 and OP1 are always evaluated
     before the guard branch; otherwise OP1's SAVE_EXPR would be initialized
     only on the OP0 == MIN path yet read on the normal-division path.  */
  tree cond = build2_loc (loc, TRUTH_AND_EXPR, boolean_type_node,
			  min_val, neg_one);
  return build_implicit_op_guard (loc, sem, op0, cond, div_result,
				  "signed division overflow");
}

/* P3100: guard an integer shift SHIFT_RESULT (OP0 shifted by OP1) whose shift
   amount may be negative or >= the width of the promoted left operand -- core-
   language UB ({expr.shift.neg.and.width}).  Returns
   `op0, (op1 < 0 || op1 >= width ? <reaction> : shift_result)`.  */

tree
build_implicit_shift_check (tree ARG_UNUSED (fndecl), location_t loc,
			    contract_evaluation_semantic sem,
			    tree op0, tree op1, tree shift_result)
{
  gcc_checking_assert (sem != CES_ASSUME);

  tree optype = TREE_TYPE (op1);
  tree width = build_int_cst (optype, TYPE_PRECISION (TREE_TYPE (op0)));
  tree cond = build2_loc (loc, GE_EXPR, boolean_type_node, op1, width);
  if (!TYPE_UNSIGNED (optype))
    {
      tree neg = build2_loc (loc, LT_EXPR, boolean_type_node, op1,
			     build_int_cst (optype, 0));
      cond = build2_loc (loc, TRUTH_ORIF_EXPR, boolean_type_node, neg, cond);
    }
  return build_implicit_op_guard (loc, sem, op0, cond, shift_result,
				  "shift count out of range");
}

/* P3100 langhook (LANG_HOOKS_BUILD_IMPLICIT_BOUNDS_CHECK): build the implicit
   array-bounds guard for a site (FNDECL + LOC) into an array of statically-
   known size.  INDEX is the (integer) subscript, or the byte offset of an
   indirection; BOUND is its first out-of-range value (for a subscript, the
   array size, adjusted by the caller for a one-past address-of context).
   GROUP is the check: ub:expr.add.out.of.bounds.known.subscript for a
   subscript, or ub:expr.unary.dereference.invalid.known for an indirection
   past the end of the array.  Resolves the evaluation semantic for GROUP and
   returns a replacement index expression `i, (out_of_range ? 0 : i)` --
   redirecting an out-of-range index to the defined valid index 0 and running
   the configured reaction -- or NULL_TREE for the assume semantic (no guard,
   raw index).  Called from the c-family array-ref walk so the C++ reaction can
   be built without linking to cc1.  */

tree
cp_build_implicit_bounds_check (tree fndecl, location_t loc, tree index,
				tree bound, const char *group)
{
  if (!flag_contracts_p3100 || fndecl == NULL_TREE)
    return NULL_TREE;

  contract_evaluation_semantic sem
    = resolve_implicit_contract_semantic (fndecl, loc, group);
  if (sem == CES_ASSUME)
    return NULL_TREE;

  const char *comment
    = (strcmp (group, "ub:expr.unary.dereference.invalid.known") == 0
       ? "indirection past the end of an array"
       : "array subscript out of bounds");

  /* Evaluate the index once and test it against the bound as unsigned, so a
     negative index (which wraps to a large value) is caught as well.  */
  tree i = save_expr (index);
  tree utype = unsigned_type_for (TREE_TYPE (i));
  tree cond = build2_loc (loc, GE_EXPR, boolean_type_node,
			  fold_convert (utype, i), fold_convert (utype, bound));
  return build_implicit_op_guard (loc, sem, i, cond, i, comment);
}

/* P3100: the offset of the pointer arithmetic P + INTOP or P - INTOP (CODE),
   in sizetype as pointer_int_sum widens it, negated for -.  */

tree
build_implicit_float_cast_check (tree ARG_UNUSED (fndecl), location_t loc,
				 contract_evaluation_semantic sem,
				 tree expr, tree converted)
{
  gcc_checking_assert (sem != CES_ASSUME);

  tree cond = ubsan_float_cast_overflow_predicate (TREE_TYPE (converted), expr);
  if (cond == NULL_TREE)
    return converted;
  return build_implicit_op_guard (loc, sem, expr, cond, converted,
				  "floating-point to integer conversion "
				  "out of range");
}

/* P3100: guard an integer/enumeration -> enumeration conversion CONVERTED (of
   the source value EXPR) whose value may be outside ENUMTYPE's value range --
   core-language UB for a non-fixed-underlying-type enum
   ({expr.static.cast.enum.outside.range}: [expr.static.cast]/8).  EXPR must be
   a single-evaluation form (SAVE_EXPR) shared with CONVERTED.  Returns
   `expr, (out_of_range ? <reaction / 0> : converted)`; the substituted value 0
   is always within an enumeration's value range.  When ENUMTYPE's value range
   spans its whole storage mode there are no invalid values, so CONVERTED is
   returned unchanged (mirrors instrument_bool_enum_load's precision guard).  */

static const char *
contract_dynamic_name (const_tree contract)
{
  tree d = CONTRACT_DYNAMIC (contract);
  if (!d)
    return NULL;
  return IDENTIFIER_POINTER (TREE_PURPOSE (d));
}

static unsigned char
contract_dynamic_linkage (const_tree contract)
{
  tree d = CONTRACT_DYNAMIC (contract);
  gcc_checking_assert (d);
  return (unsigned char) ((tree_to_uhwi (TREE_VALUE (d)) >> 1) & 0x7f);
}

/* The semantic the configuration entry that selected CONTRACT's dynamic
   selector specifies, before it is adjusted to the allowed set or passed
   through the label's compute_semantic: what the selector's weak default
   returns (D3595R1).  */

static contract_evaluation_semantic
contract_dynamic_configured_semantic (const_tree contract)
{
  tree d = CONTRACT_DYNAMIC (contract);
  gcc_checking_assert (d);
  return (contract_evaluation_semantic) (tree_to_uhwi (TREE_VALUE (d)) >> 8);
}

static bool
contract_dynamic_provideweak (const_tree contract)
{
  tree d = CONTRACT_DYNAMIC (contract);
  gcc_checking_assert (d);
  return (tree_to_uhwi (TREE_VALUE (d)) & 1) != 0;
}

/* The type std::contracts::NAME_ID if <contracts> has declared it, else
   NULL_TREE.  Unlike lookup_std_contracts_type, this only looks: it never
   injects a placeholder class, which would conflict with the header's own
   declaration of anything that is not a class (evaluation_semantic and
   violation_handled are enumerations) when <contracts> is included later in
   the TU.  */

static tree
find_std_contracts_type (tree name_id)
{
  tree ns = lookup_qualified_name (std_node, get_identifier ("contracts"));
  if (TREE_CODE (ns) != NAMESPACE_DECL)
    return NULL_TREE;
  tree decl = lookup_qualified_name (ns, name_id, LOOK_want::TYPE);
  return TREE_CODE (decl) == TYPE_DECL ? TREE_TYPE (decl) : NULL_TREE;
}

/* The return type of a dynamic-selection function: the real
   std::contracts::evaluation_semantic when the header is in scope, else
   the ABI-compatible uint16.  The return type is not part of the mangled
   name, so either choice binds to the same symbol.  */

static tree
dynamic_selector_return_type ()
{
  tree t = find_std_contracts_type (get_identifier ("evaluation_semantic"));
  if (t && t != error_mark_node && TREE_CODE (t) == ENUMERAL_TYPE)
    return t;
  return short_unsigned_type_node;
}

/* Map from selector IDENTIFIER_NODE to its FUNCTION_DECL, so we build at
   most one decl per unique name per TU.  */
static GTY(()) hash_map<tree, tree, pch_traits> *dynamic_selector_decls;

/* Selectors whose weak definition must be emitted at end of TU.  Each
   element is a TREE_LIST: PURPOSE=FUNCTION_DECL, VALUE=INTEGER_CST
   configured semantic, which the weak definition returns.  */
static GTY(()) vec<tree, va_gc> *pending_weak_selectors;

/* Resolve or synthesize the NAMESPACE_DECL designated by the leading
   (namespace) components of a P3595 "C++"-linkage selector name.  NAME is
   the full qualified string (e.g. "mylib::detail::sel"); on return, *FN_ID
   is the IDENTIFIER_NODE of the final (function) component and the returned
   tree is the innermost enclosing NAMESPACE_DECL (global_namespace for a
   bare identifier).  A component that does not yet name a namespace is
   created via push_namespace, so the weak definition can be emitted there.  */

static tree
resolve_dynamic_selector_namespace (const char *name, tree *fn_id)
{
  /* The qualified name is resolved from the global namespace.  push_namespace
     starts from the current scope, and the check is built from cp_genericize
     inside finish_function, while the namespace of the function being
     finished is still current: resolving from there would find or synthesize
     the selector inside that namespace and mangle it so (GCC-655).  A
     member function finished in its class has the class scope current as
     well, so go to the top level unconditionally.  */
  const char *sep = strstr (name, "::");
  if (!sep)
    {
      /* Bare identifier: global namespace.  */
      *fn_id = get_identifier (name);
      return global_namespace;
    }

  push_to_top_level ();

  /* Push each leading component; push_namespace resolves an existing
     namespace of that name in the current scope or creates a new one, and
     leaves it as current_namespace.  We record how many we pushed so we can
     pop back out to where we started.  */
  int pushed = 0;
  const char *comp = name;
  const char *next;
  while ((next = strstr (comp, "::")) != NULL)
    {
      tree comp_id = get_identifier_with_length (comp, next - comp);
      push_namespace (comp_id);
      pushed++;
      comp = next + 2;
    }

  /* COMP now points at the final (function) component.  */
  *fn_id = get_identifier (comp);
  tree ns = current_namespace;

  while (pushed-- > 0)
    pop_namespace ();

  pop_from_top_level ();

  return ns;
}

/* Look up the P3595 "C++"-linkage selector NAME as a qualified name from
   the global namespace, following inline namespaces as qualified lookup
   does, and return the declared function with the selector's signature
   (returning RET_TYPE, no parameters) that it names, or NULL_TREE.  Only
   declarations visible at the point of the check are found.  */

static tree
lookup_declared_dynamic_selector (const char *name, tree ret_type)
{
  tree scope = global_namespace;
  const char *comp = name;
  const char *next;
  while ((next = strstr (comp, "::")) != NULL)
    {
      tree comp_id = get_identifier_with_length (comp, next - comp);
      scope = lookup_qualified_name (scope, comp_id, LOOK_want::NAMESPACE,
				     /*complain=*/false);
      if (TREE_CODE (scope) != NAMESPACE_DECL)
	return NULL_TREE;
      scope = ORIGINAL_NAMESPACE (scope);
      comp = next + 2;
    }

  tree fns = lookup_qualified_name (scope, get_identifier (comp),
				    LOOK_want::NORMAL, /*complain=*/false);
  if (fns == error_mark_node)
    return NULL_TREE;
  for (tree fn : lkp_range (fns))
    if (TREE_CODE (fn) == FUNCTION_DECL
	&& DECL_NAMESPACE_SCOPE_P (fn)
	&& TYPE_ARG_TYPES (TREE_TYPE (fn)) == void_list_node
	&& same_type_p (TREE_TYPE (TREE_TYPE (fn)), ret_type))
      return fn;
  return NULL_TREE;
}

/* True if TYPE is std::contracts::evaluation_semantic, possibly declared
   in an inline namespace of std::contracts or of std.  */

static bool
evaluation_semantic_type_p (tree type)
{
  type = TYPE_MAIN_VARIANT (type);
  if (TREE_CODE (type) != ENUMERAL_TYPE
      || !TYPE_IDENTIFIER (type)
      || !id_equal (TYPE_IDENTIFIER (type), "evaluation_semantic"))
    return false;
  tree ctx = CP_TYPE_CONTEXT (type);
  while (ctx && TREE_CODE (ctx) == NAMESPACE_DECL
	 && DECL_NAMESPACE_INLINE_P (ctx))
    ctx = CP_DECL_CONTEXT (ctx);
  if (!ctx || TREE_CODE (ctx) != NAMESPACE_DECL
      || !DECL_NAME (ctx) || !id_equal (DECL_NAME (ctx), "contracts"))
    return false;
  ctx = CP_DECL_CONTEXT (ctx);
  while (TREE_CODE (ctx) == NAMESPACE_DECL && DECL_NAMESPACE_INLINE_P (ctx))
    ctx = CP_DECL_CONTEXT (ctx);
  return ctx == std_node;
}

/* Warn about each declaration this translation unit makes with the
   qualified name NAME of a "C++"-linkage P3595 dynamic selector but not the
   selector's type, std::contracts::evaluation_semantic ().  The selector is
   called with that type whatever the declaration says.  A function of that
   name with parameters, or a template, has another symbol and is left
   alone; a variable of that name cannot be the selector.  */

static void
diagnose_mistyped_dynamic_selector (const char *name)
{
  tree scope = global_namespace;
  const char *comp = name;
  const char *next;
  while ((next = strstr (comp, "::")) != NULL)
    {
      tree comp_id = get_identifier_with_length (comp, next - comp);
      scope = lookup_qualified_name (scope, comp_id, LOOK_want::NAMESPACE,
				     /*complain=*/false);
      if (TREE_CODE (scope) != NAMESPACE_DECL)
	return;
      scope = ORIGINAL_NAMESPACE (scope);
      comp = next + 2;
    }

  tree found = lookup_qualified_name (scope, get_identifier (comp),
				      LOOK_want::NORMAL, /*complain=*/false);
  if (found == error_mark_node)
    return;
  for (tree decl : lkp_range (found))
    {
      if (!DECL_NAMESPACE_SCOPE_P (decl))
	continue;
      if (TREE_CODE (decl) == FUNCTION_DECL)
	{
	  if (TYPE_ARG_TYPES (TREE_TYPE (decl)) != void_list_node
	      || undeduced_auto_decl (decl)
	      || evaluation_semantic_type_p (TREE_TYPE (TREE_TYPE (decl))))
	    continue;
	  warning_at (DECL_SOURCE_LOCATION (decl), OPT_Wcontract_configuration,
		      "function %qD has the name of the dynamic contract "
		      "selector %qs in the contract configuration, but its "
		      "type %qT is not "
		      "%<std::contracts::evaluation_semantic ()%>",
		      decl, name, TREE_TYPE (decl));
	}
      else if (VAR_P (decl))
	warning_at (DECL_SOURCE_LOCATION (decl), OPT_Wcontract_configuration,
		    "variable %qD has the name of the dynamic contract "
		    "selector %qs in the contract configuration, but it is "
		    "not a function of type "
		    "%<std::contracts::evaluation_semantic ()%>",
		    decl, name);
    }
}

/* Return the FUNCTION_DECL for the P3595 dynamic-selection function NAME.
   LINKAGE selects how NAME is interpreted:

   - CDL_CXX ("C++"): NAME is a (possibly fully-qualified) C++ name.  The
     enclosing namespaces are resolved/synthesized and the FUNCTION_DECL is
     built with the innermost NAMESPACE_DECL as DECL_CONTEXT and C++ language,
     so normal C++ mangling applies (e.g. "mylib::contract_semantic" ->
     _ZN5mylib17contract_semanticEv).

   - CDL_C ("C"): NAME is used verbatim as the assembler symbol (via
     SET_DECL_ASSEMBLER_NAME) with C language, so no mangling is applied.
     This lets a user target any symbol, including a mangled C++ symbol.

   Decls are cached (deduplicated) per unique NAME string per TU.  When
   PROVIDEWEAK, schedule a weak definition returning DEF_SEM, the configured
   semantic, to be emitted once for this name at end of TU.  */

static tree
get_dynamic_selector_decl (const char *name, unsigned char linkage,
			   bool provideweak,
			   contract_evaluation_semantic def_sem)
{
  /* Key the cache by the full NAME string: distinct qualified names (or
     distinct verbatim C symbols) map to distinct decls.  */
  tree key = get_identifier (name);

  hash_map_maybe_create<hm_ggc> (dynamic_selector_decls, 8);

  if (tree *cached = dynamic_selector_decls->get (key))
    return *cached;

  tree ret_type = dynamic_selector_return_type ();
  tree fntype = build_function_type_list (ret_type, NULL_TREE);

  tree fndecl;
  if (linkage == CDL_C)
    {
      /* Verbatim C symbol: a global-scope decl whose assembler name is NAME
	 exactly, with C language so mangling is suppressed.  */
      tree fn_id = get_identifier (name);
      fndecl = build_lang_decl_loc (BUILTINS_LOCATION, FUNCTION_DECL,
				    fn_id, fntype);
      DECL_CONTEXT (fndecl) = FROB_CONTEXT (global_namespace);
      SET_DECL_LANGUAGE (fndecl, lang_c);
      SET_DECL_ASSEMBLER_NAME (fndecl, get_identifier (name));
    }
  else
    {
      /* C++ name, possibly qualified: build in the resolved namespace with
	 C++ language so the symbol mangles normally.  When the name finds a
	 declared selector (possibly through an inline namespace, possibly
	 with an abi_tag), take its context and symbol instead.  */
      tree fn_id;
      tree ns = resolve_dynamic_selector_namespace (name, &fn_id);
      tree decl = lookup_declared_dynamic_selector (name, ret_type);
      fndecl = build_lang_decl_loc (BUILTINS_LOCATION, FUNCTION_DECL,
				    fn_id, fntype);
      DECL_CONTEXT (fndecl)
	= decl ? CP_DECL_CONTEXT (decl) : FROB_CONTEXT (ns);
      SET_DECL_LANGUAGE (fndecl, lang_cplusplus);
      if (decl)
	SET_DECL_ASSEMBLER_NAME (fndecl, DECL_ASSEMBLER_NAME (decl));
    }

  TREE_PUBLIC (fndecl) = true;
  DECL_EXTERNAL (fndecl) = true;
  DECL_ARTIFICIAL (fndecl) = true;

  dynamic_selector_decls->put (key, fndecl);

  if (provideweak)
    vec_safe_push (pending_weak_selectors,
		   build_tree_list (fndecl,
				    build_int_cst (ret_type, (int) def_sem)));

  return fndecl;
}

/* Emit the scheduled weak definitions of dynamic-selection functions.
   Called at end of TU from maybe_emit_violation_handler_wrappers.  Each
   weak definition simply returns the semantic its configuration entry
   specifies (D3595R1), so a program links and runs with no user-supplied
   selector, while a strong user definition overrides it at link time.

   That is the configured semantic as written, not the contract's
   compile-time semantic: each check adjusts the value its selector returns
   to the allowed set and passes it through the label's compute_semantic
   (transform_semantic), so the weak default reproduces the compile-time
   semantic only if it has not had them applied already -- a
   compute_semantic need not be idempotent.  It is also the only value
   that can serve every contract sharing the symbol: the label and the
   allowed set are per contract, the weak definition per symbol, and other
   translation units, compiled with other flags, may emit the definition
   the linker keeps.  Two configuration entries that name one selector
   with different semantics get the first one's here (the first contract
   to use the selector); give them the same semantic.  First warn about
   any declaration of a configured selector's name with the wrong
   type.  */

static void
emit_pending_weak_selectors ()
{
  /* Every selector the configuration names is checked against the
     declarations of its name, whether or not a contract used it.  */
  contract_config_for_each_cxx_selector (diagnose_mistyped_dynamic_selector);

  if (!pending_weak_selectors)
    return;

  /* Symbols we have already emitted a weak definition for, keyed by the final
     assembler name.  The per-name decl cache (dynamic_selector_decls) already
     collapses two config entries that name the same selector with the same
     string into one pending entry, so one weak is emitted.  This set adds the
     final backstop: two config entries whose *distinct* name strings resolve to
     the same symbol (e.g. a "C++" name mylib::sel and a "C" verbatim mangled
     _ZN5mylib3selEv) still yield at most one weak definition, never a
     duplicate-symbol link error.  */
  hash_set<tree> emitted_asm_names;

  unsigned i;
  tree elt;
  FOR_EACH_VEC_ELT (*pending_weak_selectors, i, elt)
    {
      tree fndecl = TREE_PURPOSE (elt);
      tree def_val = TREE_VALUE (elt);

      /* Already emitted a weak for this exact symbol via another entry.  */
      if (emitted_asm_names.contains (DECL_ASSEMBLER_NAME (fndecl)))
	continue;

      /* If the user (or some other definition) already provides a strong
	 definition of this selector in this TU, emitting our weak definition
	 too would produce two definitions of the same symbol.  Detect this by
	 the *assembler* name: for a "C++" qualified selector the user's strong
	 definition lives in the resolved namespace and mangles to this symbol;
	 for a "C" verbatim selector the strong definition is any C++ function
	 whose mangled name happens to equal this symbol.  A symtab lookup by
	 assembler name catches both, and works for non-global namespaces, so
	 it needs no source-level (namespace-scoped) name lookup.  */
      tree asm_name = DECL_ASSEMBLER_NAME (fndecl);
      bool user_defined = false;
      for (symtab_node *node = symtab_node::get_for_asmname (asm_name);
	   node; node = node->next_sharing_asm_name)
	{
	  tree decl = node->decl;
	  if (decl != fndecl
	      && TREE_CODE (decl) == FUNCTION_DECL
	      && DECL_INITIAL (decl) != NULL_TREE
	      && DECL_ASSEMBLER_NAME_SET_P (decl)
	      && DECL_ASSEMBLER_NAME (decl) == asm_name)
	    {
	      user_defined = true;
	      break;
	    }
	}
      if (user_defined)
	continue;

      emitted_asm_names.add (DECL_ASSEMBLER_NAME (fndecl));

      DECL_EXTERNAL (fndecl) = false;
      DECL_INITIAL (fndecl) = error_mark_node;
      DECL_RESULT (fndecl) = NULL_TREE;

      start_preparsed_function (fndecl, NULL_TREE, SF_DEFAULT | SF_PRE_PARSED);
      tree body = begin_function_body ();
      tree compound_stmt = begin_compound_stmt (BCS_FN_BODY);
      finish_return_stmt (def_val);
      finish_compound_stmt (compound_stmt);
      finish_function_body (body);
      tree fn = finish_function (false);
      declare_weak (fn);
      expand_or_defer_fn (fn);
    }

  vec_free (pending_weak_selectors);
  pending_weak_selectors = NULL;
}

/* The throwing semantics: observe and enforce, the two whose evaluation
   may let an exception leave the check -- the violation handler, which may
   throw, is called through an entry point that lets it, as is the report of
   an exception from the predicate.  No other semantic can: noexcept_observe
   and noexcept_enforce (P4298) call the handler as noexcept, so an
   exception from it terminates the program; quick_enforce traps inline and
   calls none; ignore and assume evaluate nothing at run time (on the P3595
   dynamic path assume is evaluated as ignore), and neither do the P4405
   prove and diagnose.  */

static const uint16_t contract_throwing_semantics
  = (1 << CES_OBSERVE) | (1 << CES_ENFORCE);

/* P3595, DECISIONS.md E137 and E138: true if MASK, the semantics a
   dynamically selected contract may be evaluated with -- its label's
   allowed_semantics facet intersected with the flag-gated base set
   (make_contract_query) -- holds no throwing semantic
   (contract_throwing_semantics).  A dynamic selection must not then bring
   one in.  Each semantic it is evaluated with is in MASK
   (transform_semantic: the clamp takes a selected observe or enforce to an
   allowed noexcept counterpart, and a compute_semantic result outside MASK
   is an invalid selection, as the static path rejects it).  An invalid
   selection is not reported through the enforce entry point
   (emit_dynamic_dispatch): with -fcontracts-p4298 it is reported through
   the noexcept_enforce one, so the handler sees noexcept_enforce and an
   exception from it terminates; without it, where noexcept_enforce is not
   available, it is a quick_enforce trap, whether or not MASK allows
   quick_enforce.  Either way no exception leaves the check.  Never for a
   contract with no transforming label (contract_label_transforms_p), nor
   in C, which has no labels: the base set always holds both throwing
   semantics.  */

static bool
contract_semantics_nothrow_p (uint16_t mask)
{
  return !(mask & contract_throwing_semantics);
}

/* Compute T(RAW) at compile time for the P3595 dynamic-dispatch transform:
   clamp RAW to the label's allowed set via the resolution fallback order, then
   apply the compute_semantic facet.  Sets *OK to false (and returns
   CES_INVALID) when the clamp finds no allowed semantic or the compute_semantic
   result is disallowed -- stage 2 turns that sentinel into a runtime enforced
   violation.  This mirrors contract_semantic_best_fit + compute_semantic
   in contracts-config.cc / this file, but never issues a compile-time
   error.  A compute_semantic result outside the allowed set -- also an
   observe or enforce when the set holds no throwing semantic
   (contract_semantics_nothrow_p) -- is an invalid selection, as the static
   path rejects it (DECISIONS.md E138); the clamp already takes a selected
   throwing semantic to an allowed noexcept counterpart where it can.  The
   selector's weak default returns the configured semantic, to which
   neither has been applied, through the same transform, so it is evaluated
   with the compile-time semantic.

   D4405: the selected semantic is a runtime evaluation's, adjusted to the
   runtime allowed set (contract_runtime_allowed_mask) as the compile-time
   semantic is, so a selected prove or diagnose is taken as ignore, as the
   configured one would be moved to the analysis evaluation, and the walk
   never reaches either.  A compute_semantic prove or diagnose likewise
   takes the evaluation to ignore, provided it is in the analysis allowed
   set and ignore in the runtime one.  */

static contract_evaluation_semantic
transform_semantic (tree contract, tree fndecl,
		    contract_evaluation_semantic raw, bool *ok)
{
  contract_query q = make_contract_query (contract, fndecl);
  uint16_t mask = q.allowed_mask;
  uint16_t runtime_mask = contract_runtime_allowed_mask (mask, false);
  uint16_t analysis_mask = contract_analysis_allowed_mask (mask);

  /* Stage: clamp to the allowed set using the best-fit safety-level search.  */
  if (contract_semantic_proving_p (raw))
    raw = CES_IGNORE;
  uint16_t s = (uint16_t) contract_semantic_best_fit (raw, runtime_mask);

  /* Stage: apply compute_semantic (returns CES_INVALID if disallowed).  */
  if (s != CES_INVALID)
    s = apply_compute_semantic_value (CONTRACT_LABEL (contract), s, mask,
				      EXPR_LOCATION (contract));
  if (contract_semantic_proving_p ((contract_evaluation_semantic) s))
    s = ((analysis_mask & (1 << s)) ? (uint16_t) CES_IGNORE
	 : (uint16_t) CES_INVALID);

  *ok = (s != CES_INVALID) && (runtime_mask & (1 << s)) != 0;
  return (contract_evaluation_semantic) (*ok ? s : (uint16_t) CES_INVALID);
}

/* Does CONTRACT's label transform the raw selector value non-trivially?
   True when a compute_semantic facet is present or allowed_semantics narrows
   the set the flags allow -- in which case the P3595 dynamic path must emit
   the two-stage map/dispatch.  When false each value a selector may return
   only needs adjusting to the flag-gated set (dynamic_adjusted_semantic), and
   the single-stage cascade is used.  */

static bool
contract_label_transforms_p (tree contract, tree fndecl)
{
  tree label = CONTRACT_LABEL (contract);
  if (label_has_compute_semantic (label))
    return true;
  contract_query q = make_contract_query (contract, fndecl);
  return q.allowed_mask != contract_base_allowed_mask ();
}

/* The semantic a check with no transforming label evaluates with when its
   dynamic selector returns RAW, one of dynamic_raw_semantics: RAW adjusted to
   the flag-gated set (D3595R1, "Adjusting a Semantic to the Allowed Set"),
   which always has the four C++26 semantics, so the result is valid.  An
   allowed assume, prove or diagnose is evaluated as ignore: a dynamically
   selected contract is never diagnosed at compile time.  */

static contract_evaluation_semantic
dynamic_adjusted_semantic (int raw)
{
  contract_evaluation_semantic s
    = contract_semantic_best_fit ((contract_evaluation_semantic) raw,
				  contract_base_allowed_mask ());
  return contract_semantic_has_no_runtime_check (s) ? CES_IGNORE : s;
}

/* Emit DEST = T(RAW), where RAW is the value a P3595 dynamic selector
   returned for CONTRACT and T maps each value a selector may return to the
   semantic to evaluate with -- the label's transform if TRANSFORMS, and the
   adjustment to the flag-gated set otherwise -- or to CES_INVALID when the
   result is not allowed or RAW is not a value a selector may return.  */

static void
emit_dynamic_semantic_map (tree contract, tree raw, tree ret_type, tree dest,
			   bool transforms)
{
  location_t loc = EXPR_LOCATION (contract);

  auto_vec<tree, 7> map_ifs;
  for (int s : dynamic_raw_semantics ())
    {
      tree cmp = build2 (EQ_EXPR, boolean_type_node, raw,
			 build_int_cst (ret_type, s));
      tree if_stmt = begin_if_stmt ();
      finish_if_stmt_cond (cmp, if_stmt);
      bool ok = true;
      contract_evaluation_semantic eff_sem
	= (transforms
	   ? transform_semantic (contract, current_function_decl,
				 (contract_evaluation_semantic) s, &ok)
	   : dynamic_adjusted_semantic (s));
      /* A dynamically-resolved "assume" cannot inform the optimizer -- the
	 predicate is never evaluated on this path, so there is nothing to
	 assume from -- and the only universally-correct behavior is to do
	 exactly what "ignore" does: no check, no violation, continue.  Map
	 it to CES_IGNORE so stage 2's ignore arm handles it.  The transform
	 yields assume only when -fcontracts-allow-assume put it in the
	 allowed set; otherwise it was adjusted away.  The same goes for the
	 P4405 prove and diagnose: the selection happens at run time, where
	 there is nothing to diagnose.  */
      int eff_val = !ok ? (int) CES_INVALID
		  : contract_semantic_has_no_runtime_check (eff_sem)
		  ? (int) CES_IGNORE
		  : (int) eff_sem;
      finish_expr_stmt
	(cp_build_modify_expr (loc, dest, NOP_EXPR,
			       build_int_cst (short_unsigned_type_node,
					      eff_val),
			       tf_warning_or_error));
      finish_then_clause (if_stmt);
      begin_else_clause (if_stmt);
      map_ifs.safe_push (if_stmt);
    }
  /* Default (unknown raw value): CES_INVALID.  */
  finish_expr_stmt
    (cp_build_modify_expr (loc, dest, NOP_EXPR,
			   build_int_cst (short_unsigned_type_node,
					  (int) CES_INVALID),
			   tf_warning_or_error));
  for (int i = map_ifs.length () - 1; i >= 0; i--)
    {
      finish_else_clause (map_ifs[i]);
      finish_if_stmt (map_ifs[i]);
    }
}

/* Emit the stage-2 dispatch of CONTRACT, a dynamically selected contract,
   on DISPATCH_VAL, of type DISPATCH_TYPE: the check for each semantic in
   dynamic_dispatch_semantics, each emitted once, and an enforced violation,
   whose data block is at DATA_ADDR, for any other value.  If RAW,
   DISPATCH_VAL is the selector's own value, and each arm also takes the
   values dynamic_adjusted_semantic maps to its semantic; otherwise it has
   already been mapped (emit_dynamic_semantic_map).  Nested if-statements,
   opened outermost-first and closed innermost-first so that their else
   clauses nest into the enforced-violation default.  */

static void
emit_dynamic_dispatch (tree contract, tree dispatch_val, tree dispatch_type,
		       tree data_addr, bool raw)
{
  auto_vec<tree, 6> if_stmts;
  for (int s : dynamic_dispatch_semantics ())
    {
      tree cmp = NULL_TREE;
      for (int r : dynamic_raw_semantics ())
	{
	  if (raw ? dynamic_adjusted_semantic (r) != s : r != s)
	    continue;
	  tree eq = build2 (EQ_EXPR, boolean_type_node, dispatch_val,
			    build_int_cst (dispatch_type, r));
	  cmp = (cmp ? build2 (TRUTH_ORIF_EXPR, boolean_type_node, cmp, eq)
		 : eq);
	}
      tree if_stmt = begin_if_stmt ();
      finish_if_stmt_cond (cmp, if_stmt);
      tree body = emit_check_for_semantic (contract,
					   (contract_evaluation_semantic) s,
					   data_addr);
      if (body && body != void_node && body != error_mark_node)
	add_stmt (body);
      finish_then_clause (if_stmt);
      begin_else_clause (if_stmt);
      if_stmts.safe_push (if_stmt);
    }

  /* Final else: an unknown value (raw dispatch) or the CES_INVALID sentinel
     (two-stage map) yields an enforced violation.  When the allowed set
     holds no throwing semantic (contract_semantics_nothrow_p) it is a
     noexcept one with -fcontracts-p4298, and a quick_enforce trap without
     it, whatever the set allows (DECISIONS.md E138).  */
  contract_query q = make_contract_query (contract, current_function_decl);
  contract_evaluation_semantic invalid_sem = CES_ENFORCE;
  if (contract_semantics_nothrow_p (q.allowed_mask))
    invalid_sem = flag_contracts_p4298 ? CES_NOEXCEPT_ENFORCE : CES_QUICK;
  tree def_body = emit_enforced_violation (contract, data_addr, invalid_sem);
  if (def_body && def_body != error_mark_node)
    add_stmt (def_body);

  /* Close the else clauses / if statements, innermost first.  */
  for (int i = if_stmts.length () - 1; i >= 0; i--)
    {
      finish_else_clause (if_stmts[i]);
      finish_if_stmt (if_stmts[i]);
    }
}

/* If CONTRACT is one of the statements recorded in dynamic_selection_sites
   for a dynamically selected postcondition with captures, return what it
   becomes: the statement that calls the selector and stores the mapped
   semantic, or the postcondition check that dispatches on the stored
   semantic without calling the selector again.  Otherwise return
   NULL_TREE.  */

static tree
build_dynamic_capture_selection (tree contract)
{
  tree *site_p = hash_map_safe_get (dynamic_selection_sites, contract);
  if (!site_p)
    return NULL_TREE;
  tree dest = TREE_VALUE (*site_p);

  tree cc_bind = build3 (BIND_EXPR, void_type_node, NULL, NULL, NULL);
  BIND_EXPR_BODY (cc_bind) = push_stmt_list ();
  if (integer_zerop (TREE_PURPOSE (*site_p)))
    {
      tree fndecl
	= get_dynamic_selector_decl (contract_dynamic_name (contract),
				     contract_dynamic_linkage (contract),
				     contract_dynamic_provideweak (contract),
				     contract_dynamic_configured_semantic
				       (contract));
      tree raw = save_expr (build_call_n (fndecl, 0));
      emit_dynamic_semantic_map (contract, raw, TREE_TYPE (TREE_TYPE (fndecl)),
				 dest,
				 contract_label_transforms_p
				   (contract, current_function_decl));
    }
  else
    {
      emit_dynamic_dispatch (contract, dest, TREE_TYPE (dest),
			     build_contract_data_block_address (contract),
			     /*raw=*/false);
    }
  BIND_EXPR_BODY (cc_bind) = pop_stmt_list (BIND_EXPR_BODY (cc_bind));
  return cc_bind;
}

/* True if every translation unit's evaluation of CONTRACT, a callee-side
   one, lets the optimizers take its predicate to hold where it continues:
   its label's allowed_semantics facet, which every translation unit sees in
   the declaration, admits only the terminating semantics and assume --
   none of ignore, observe and noexcept_observe, nor diagnose (which only
   warns) nor prove (terminating only where its proof is known sound, which
   is known only for this translation unit's own evaluations).  Never for a
   caller-side evaluation in a wrapper: ignore is always admitted there, so
   another translation unit may leave it out.  */

static bool
contract_premise_settled_p (tree contract)
{
  if (current_function_decl
      && DECL_CONTRACT_WRAPPER (current_function_decl)
      && !wrapper_for_virtual_dispatch_p (current_function_decl))
    return false;
  tree mask_tree = CONTRACT_ALLOWED_MASK (contract);
  if (!mask_tree)
    return false;
  uint16_t mask = (uint16_t) tree_to_uhwi (mask_tree);
  const uint16_t continuing
    = ((1 << CES_IGNORE) | (1 << CES_OBSERVE) | (1 << CES_NOEXCEPT_OBSERVE)
       | (1 << CES_PROVE) | (1 << CES_DIAGNOSE));
  return mask != 0 && !(mask & continuing);
}

/* P4405 proving hook: tell the strategies when the check of CONTRACT under
   SEMANTIC, if a plain one, lets the optimizers take its predicate to hold
   where the function continues -- enforce and noexcept_enforce call a
   noreturn entry point, quick_enforce traps inline and assume is an
   assumption -- unless every translation unit's evaluation would
   (contract_proof_note_premise).  */

static void
note_contract_premise (tree contract, contract_evaluation_semantic semantic)
{
  if (contract_dynamic_name (contract))
    return;
  switch (semantic)
    {
    case CES_ENFORCE:
    case CES_NOEXCEPT_ENFORCE:
    case CES_QUICK:
    case CES_ASSUME:
      break;
    default:
      return;
    }
  if (!contract_premise_settled_p (contract))
    contract_proof_note_premise (current_function_decl);
}

/* Genericize a CONTRACT tree, but do not attach it to the current context,
   the caller is responsible for that.
   This is called during genericization.  */

static tree build_runtime_contract_check (tree,
					  contract_evaluation_semantic, bool);

tree
build_contract_check (tree contract)
{
  contract_evaluation_semantic semantic;
  contract_evaluation_semantic analysis
    = ensure_analysis_semantic (contract, current_function_decl, &semantic);

  /* D4405: the analysis evaluation comes first, then the runtime
     evaluation (or, when the runtime semantic is ignore, nothing more).  */
  tree analysis_eval = build_analysis_evaluation (contract, analysis);
  tree check
    = build_runtime_contract_check (contract, semantic,
				    contract_analyzed_here_p (contract,
							      analysis));
  if (!analysis_eval)
    return check;
  if (!check || check == void_node)
    return analysis_eval;
  if (check == error_mark_node)
    return check;
  tree list = alloc_stmt_list ();
  append_to_statement_list_force (analysis_eval, &list);
  append_to_statement_list_force (check, &list);
  return list;
}

/* The runtime evaluation of CONTRACT, with the semantic SEMANTIC in the
   current function (build_contract_check).  ANALYZED is true if a prove or
   diagnose analysis evaluation precedes it.  */

static tree
build_runtime_contract_check (tree contract,
			      contract_evaluation_semantic semantic,
			      bool analyzed)
{
  /* Plain (non-dynamic) contract: emit the single resolved check.  */
  const char *dyn_name = contract_dynamic_name (contract);
  /* P4405 proving hook: the calls in a predicate that a check evaluates.  */
  if (dyn_name || !contract_semantic_has_no_runtime_check (semantic))
    diagnose_checked_predicate_calls (contract);
  /* P4405 proving hook: what a plain check lets the optimizers take to
     hold.  */
  note_contract_premise (contract, semantic);
  /* D4405: an ignore evaluation after a prove or diagnose analysis
     evaluation adds nothing, not even an assumption: the strategies take
     the analysis evaluation's predicate to hold where it continues.  */
  if (!dyn_name && analyzed && semantic == CES_IGNORE)
    return void_node;
  /* P4405 proving hook: a plain check, followed by what lets a strategy
     that defers evaluations assume the predicate.  */
  if (!dyn_name && contract_proof_deferrable_p (contract))
    return add_contract_proof_assumption
	     (contract, semantic, emit_check_for_semantic (contract, semantic));
  if (!dyn_name)
    return emit_check_for_semantic (contract, semantic);

  /* A postcondition with captures chooses its semantic once, where the
     captures are initialized.  */
  if (tree check = build_dynamic_capture_selection (contract))
    return check;

  /* P3595 dynamic selection.  Dispatch on the selector's runtime return value,
     emitting each distinct check body exactly once and driving an unknown value
     to an enforced violation.

     This runs during genericization, where the parser's switch machinery
     (finish_case_label et al.) is not available, so each dispatch is built
     as an if / else-if cascade comparing a value against each semantic.  The
     statement-tree if builders (begin_if_stmt ...) are the same ones the
     non-dynamic check body uses at this stage.

     When the contract's label transforms the raw value non-trivially (an
     allowed_semantics facet narrows the set, or a compute_semantic facet is
     present), a TWO-STAGE form is emitted:

       stage 1: eff = T(raw), mapping each semantic a selector may return to
		its compile-time transform T() (or CES_INVALID when the result
		is disallowed), an allowed assume to ignore, and an unknown raw
		value to CES_INVALID.
       stage 2: dispatch on eff, calling emit_check_for_semantic for each
		semantic dispatched on and emit_enforced_violation for
		CES_INVALID.

     With no transforming label stage 1 is skipped and stage 2 dispatches
     directly on the raw selector value, each arm also taking the values that
     are adjusted to its semantic (dynamic_adjusted_semantic): assume, prove
     and diagnose, allowed or not, take the ignore arm, and without
     -fcontracts-p4298 a noexcept semantic takes the arm of its throwing
     counterpart.  */
  bool provideweak = contract_dynamic_provideweak (contract);
  unsigned char linkage = contract_dynamic_linkage (contract);

  tree fndecl
    = get_dynamic_selector_decl (dyn_name, linkage, provideweak,
				 contract_dynamic_configured_semantic (contract));
  tree ret_type = TREE_TYPE (TREE_TYPE (fndecl));

  tree cc_bind = build3 (BIND_EXPR, void_type_node, NULL, NULL, NULL);
  BIND_EXPR_BODY (cc_bind) = push_stmt_list ();

  /* The violation data block is identical for every dispatch arm of this
     contract (same source location, comment, kind, ...), so build it ONCE here
     and reuse its address across all arms and the enforced-violation default,
     rather than emitting a duplicate global per arm.  */
  tree data_addr = build_contract_data_block_address (contract);

  tree raw = save_expr (build_call_n (fndecl, 0));

  bool transforms
    = contract_label_transforms_p (contract, current_function_decl);

  /* The value stage 2 dispatches on: the raw selector value, or the
     transformed "eff" temporary for the two-stage form.  */
  tree dispatch_val = raw;
  tree dispatch_type = ret_type;

  if (transforms)
    {
      /* Stage 1: eff = T(raw).  Introduce a uint16 temporary added to the
	 enclosing BIND_EXPR, then a cascade assigning T(s) for each known raw
	 value and CES_INVALID for the default (unknown) case.  */
      location_t loc = EXPR_LOCATION (contract);
      tree eff = build_decl (loc, VAR_DECL, NULL, short_unsigned_type_node);
      DECL_ARTIFICIAL (eff) = true;
      DECL_IGNORED_P (eff) = true;
      DECL_CONTEXT (eff) = current_function_decl;
      layout_decl (eff, 0);
      add_decl_expr (eff);
      DECL_CHAIN (eff) = BIND_EXPR_VARS (cc_bind);
      BIND_EXPR_VARS (cc_bind) = eff;

      emit_dynamic_semantic_map (contract, raw, ret_type, eff,
				 /*transforms=*/true);

      dispatch_val = eff;
      dispatch_type = short_unsigned_type_node;
    }

  /* Stage 2 (or the sole stage when no label transforms the value).  */
  emit_dynamic_dispatch (contract, dispatch_val, dispatch_type, data_addr,
			 /*raw=*/!transforms);

  BIND_EXPR_BODY (cc_bind) = pop_stmt_list (BIND_EXPR_BODY (cc_bind));
  return cc_bind;
}

/* P3400 [expr.contract.control]: contract_control (EXPR) is an lvalue
   denoting a constexpr object with a deduced type initialized by its
   constant-expression EXPR.  The object is made as a template parameter
   object is, so one object stands for each type and value.  In a template
   the expression is kept as a CONTRACT_CONTROL_EXPR and finished at
   instantiation.  */

tree
finish_contract_control_expr (location_t loc, tree expr)
{
  if (error_operand_p (expr))
    return error_mark_node;
  if (processing_template_decl)
    return build_min_nt_loc (loc, CONTRACT_CONTROL_EXPR, expr);

  /* The type is deduced as for `constexpr auto v = EXPR;'.  */
  expr = decay_conversion (expr, tf_warning_or_error);
  if (error_operand_p (expr))
    return error_mark_node;
  tree type = cv_unqualified (TREE_TYPE (expr));
  expr = cp_convert (type, expr, tf_warning_or_error);
  tree val = cxx_constant_value (expr);
  if (error_operand_p (val))
    return error_mark_node;
  if (TREE_CODE (val) == TARGET_EXPR)
    val = TARGET_EXPR_INITIAL (val);
  if (!reduced_constant_expression_p (val))
    {
      error_at (loc, "operand of %<contract_control%> is not a constant "
		"expression");
      return error_mark_node;
    }
  return get_template_parm_object (val, mangle_template_parm_object (val));
}

/* P3400: true if the evaluation of CONTRACT, a contract assertion of
   FNDECL or one of its precondition or postcondition specifiers evaluated
   on its behalf with the callee's semantic, is settled: whatever semantic
   each translation unit chooses for it, no definition of FNDECL behaves, for
   a caller that does not inline it, in a way another definition does not.
   A settled evaluation does not make FNDECL a refinable body (the
   refinable-body rule: cp_genericize_r and finish_function mark a function
   holding an evaluation that is not settled, so that calls which do not
   inline it use no facts derived from this translation unit's definition).

   Settled needs both directions:

   A. Facts derived from one definition's evaluation hold for every other
      definition's: every semantic the evaluation may use ends the program
      on a violation (enforce, noexcept_enforce, quick_enforce) or makes the
      violation undefined (assume), so wherever any definition continues past
      the evaluation, the predicate held there.  So none of ignore, observe,
      noexcept_observe, prove and diagnose may be allowed.

   B. No definition's evaluation has an effect visible to a caller that
      another's lacks: either no semantic it may use lets an exception leave
      FNDECL -- only enforce does: its violation handler may throw
      (noexcept_enforce calls the handler as noexcept, and quick_enforce and
      assume call none) -- or FNDECL is noexcept, where such an exception
      terminates the program instead, or enforce is the only semantic it
      may use, so every definition's evaluation may throw alike.

   So the evaluation is settled exactly when the set of semantics it may use
   is a non-empty subset of {noexcept_enforce, quick_enforce, assume}, or is
   {enforce} alone, or FNDECL is noexcept and it is a non-empty subset of
   {enforce, noexcept_enforce, quick_enforce, assume}.  The set every translation unit
   chooses from is the label's allowed_semantics facet, part of the
   declaration and so the same everywhere (CONTRACT_ALLOWED_MASK); a
   translation unit may narrow it further (contract_base_allowed_mask),
   which keeps it settled.  An assertion with no label, or whose label does
   not restrict the set, is never settled, nor is an evaluation in a
   caller-side wrapper, where ignore is always allowed
   (contract_wrapper_settled_p), nor a dynamically selected one (P3595).
   The dynamic dispatch keeps within the allowed set, and where that set
   holds no throwing semantic it brings none in, not even for an invalid
   selection (contract_semantics_nothrow_p, DECISIONS.md E137, E138), so the
   evaluation itself could be settled; but the selector is an ordinary
   external function, declared with no exception specification, that this
   definition calls and another definition need not: an exception from it
   leaves FNDECL, and what it reads and writes is visible to a caller, which
   breaks B whatever the label allows.

   Deliberately not covered (accepted, DECISIONS.md E137): a definition
   whose evaluation calls a violation handler on the way to termination
   (enforce, noexcept_enforce) may show it memory that a caller, using facts
   from a definition whose evaluation calls none (quick_enforce, assume),
   did not keep up to date -- the program ends either way; and whether the
   predicate is evaluated, with its side effects, is unspecified under every
   checking semantic ([basic.contract.eval]).  */

bool
contract_evaluation_settled_p (tree contract, tree fndecl)
{
  if (!fndecl || error_operand_p (contract))
    return false;
  if (DECL_IS_WRAPPER_FN_P (fndecl)
      && !wrapper_for_virtual_dispatch_p (fndecl))
    return false;
  tree mask_tree = CONTRACT_ALLOWED_MASK (contract);
  if (!mask_tree)
    return false;
  uint16_t mask = (uint16_t) tree_to_uhwi (mask_tree);
  uint16_t settled = ((1 << CES_NOEXCEPT_ENFORCE) | (1 << CES_QUICK)
		      | (1 << CES_ASSUME));
  if (TREE_CODE (TREE_TYPE (fndecl)) != ERROR_MARK)
    {
      tree spec = TYPE_RAISES_EXCEPTIONS (TREE_TYPE (fndecl));
      if (spec
	  && !DEFERRED_NOEXCEPT_SPEC_P (spec)
	  && !UNPARSED_NOEXCEPT_SPEC_P (spec)
	  && nothrow_spec_p (spec))
	settled |= 1 << CES_ENFORCE;
    }
  if (mask == (1 << CES_ENFORCE))
    settled |= 1 << CES_ENFORCE;
  if (mask == 0 || (mask & ~settled))
    return false;
  /* The descriptor of a dynamic selection is cached with the semantic.  */
  ensure_evaluation_semantic (contract, fndecl, false);
  return !contract_dynamic_name (contract);
}

/* P3400: true if every precondition and postcondition specifier of FNDECL,
   evaluated on its behalf with the callee's semantic, is settled
   (contract_evaluation_settled_p) -- vacuously so for a function with none.
   finish_function marks a function with specifiers that are not.  */

bool
contract_specifiers_settled_p (tree fndecl)
{
  if (processing_template_decl
      || !DECL_HAS_CONTRACTS_P (fndecl))
    return true;
  tree specs = get_fn_contract_specifiers (DECL_ORIGIN (fndecl));
  if (!specs)
    return false;
  for (tree contract : tree_vec_range (specs))
    if (!contract_evaluation_settled_p (contract, fndecl))
      return false;
  return true;
}

/* P3097, P3400: true if a call of WRAPPER, a contract wrapper, holds only
   settled evaluations (contract_evaluation_settled_p), so the refinable-body
   rule does not mark its caller for it (cp_genericize_r).  A caller-side
   wrapper never does: ignore is always allowed for a caller-side evaluation,
   so another translation unit's definition of the caller may leave it out.
   A wrapper for a virtual call (P3097) evaluates the contract assertions of
   the function it calls with their callee-side semantics.  */

bool
contract_wrapper_settled_p (tree wrapper)
{
  if (!wrapper_for_virtual_dispatch_p (wrapper))
    return false;
  tree fndecl = get_orig_func_for_wrapper (wrapper);
  if (!fndecl)
    return false;
  tree specs = get_fn_contract_specifiers (DECL_ORIGIN (fndecl));
  if (!specs)
    return true;
  for (tree contract : tree_vec_range (specs))
    if (!contract_evaluation_settled_p (contract, fndecl))
      return false;
  return true;
}

/* D4405R0, "Virtual Functions": override shadows.  A call through a base
   class B evaluates the contract assertions of B::f, which the caller
   sees, and those of the final overrider D::f, which it cannot.  So for
   each function B::f that a virtual D::f defined here overrides, directly
   or indirectly, an override shadow of D::f's body takes B::f's
   preconditions as given and attempts to prove, in order: D::f's
   preconditions at entry; D::f's postconditions at each return, each ahead
   of its own runtime check; and B::f's postconditions after all of D::f's
   checks, where the P3097 wrapper evaluates them.  Each is an analysis
   evaluation of the assertion being proven, with its own analysis
   semantic, resolved against the function it belongs to; when every one is
   ignore, there is no override shadow.  A B::f without contract assertions
   gets one too, with no premises: D::f's preconditions must then hold for
   any argument (DECISIONS.md E156).  The pair is registered with the proof
   shadows' middle end (contract_shadow_register_override), which issues
   its diagnostics, and the proving strategies see it in each query.  An
   override proof is never assumed in the real code, which also serves
   direct calls, where B::f's preconditions were never established, and the
   diagnostics are issued at D::f's definition, naming B::f (DECISIONS.md
   E155).

   In B::f's postconditions, `this' is a snapshot of D::f's `this'
   converted to the B subobject; a parameter that D::f receives by value,
   but not by invisible reference, is a snapshot of it, which is what the
   wrapper's own copy holds whatever D::f does to its parameter (E153),
   and any other parameter is D::f's own; the result name is D::f's result,
   converted to B::f's return type for a covariant return as the covariant
   thunk converts it.  The snapshots of `this' and the parameters are taken
   at entry, and every snapshot is assigned only inside a region of the
   pair's kind, so the real function, which sheds the regions before SSA,
   never uses them.

   maybe_apply_function_contracts drives this.  begin_override_shadows
   makes the pairs, D::f's preconditions' obligations and B::f's
   postconditions'; apply_postconditions places D::f's postconditions'
   ahead of their checks (add_override_post_obligations), and
   maybe_apply_function_contracts B::f's after them
   (add_override_base_post_obligations), which also places the regions at
   entry -- snapshots, premises and obligations -- for the pairs that placed
   a marker (finish_override_shadows).  A D::f without active
   postconditions gets the regions alone, or, for B::f's postconditions, a
   try/finally of their own (place_override_shadows_alone).  An obligation
   that no strategy may defer (at -O0, say, or a postcondition with
   captures) is decided at once.

   Override shadows are a proving strategy of their own,
   -fcontracts-prove-override (contract_proof_override_shadows_p;
   DECISIONS.md E157).  Without it, each pair's evaluations are obligations
   of D::f itself, with no premise from B::f: D::f's preconditions are
   decided at once, so only folding settles them; D::f's postconditions
   need no more than their own analysis evaluations; and B::f's
   postconditions, rewritten as above, are proven in D::f's ordinary
   shadow after D::f's checks.  Their diagnostics name B::f all the same.

   A virtual destructor overrides too, an implicitly defined one included
   (DECISIONS.md E152): its body for the override shadows is what it runs --
   its own statements, then the destruction of the members and bases, so
   B::~B's postconditions are proven after B::~B has run.  In a class with
   virtual bases the regions are placed only in the complete-object
   destructor (override_in_charge), which is what a delete through a base
   class runs; the base-object destructor, which leaves the virtual bases
   alone, is never the target of a virtual call.

   A base class occurring as more than one subobject gives one pair per
   subobject when B::f's contract assertions name `this', which then reads
   a different object in each, and one pair otherwise (DECISIONS.md
   E154).  */

/* cp_walk_tree callback: stop at `this', the real one or the placeholder
   of a predicate parsed on a declaration.  */

static tree
find_this_parameter_r (tree *tp, int *walk_subtrees, void *)
{
  if (TYPE_P (*tp))
    *walk_subtrees = 0;
  else if (TREE_CODE (*tp) == PARM_DECL && is_this_parameter (*tp))
    return *tp;
  return NULL_TREE;
}

/* True if a contract assertion of FN names `this', so that it reads the
   subobject a call through FN's class is made on.  */

static bool
contracts_name_this_p (tree fn)
{
  tree specs = get_fn_contract_specifiers (fn);
  if (!specs)
    return false;
  for (tree c : tree_vec_range (specs))
    {
      tree cond = CONTRACT_CONDITION (c);
      if (cond && cond != error_mark_node
	  && !CONTRACT_CONDITION_DEFERRED_P (c)
	  && cp_walk_tree_without_duplicates (&cond, find_this_parameter_r,
					      NULL))
	return true;
    }
  return false;
}

/* Push onto FNS each function that FNDECL, a member of the class of
   COMPLETE, overrides in a base class of BINFO, directly or indirectly --
   every one, not only the first on each path, which look_for_overrides
   stops at -- and onto BINFOS the base class (in FNDECL's class) it is
   found in: once per function, the first base class it is found in, or,
   when its contract assertions name `this', once per base class subobject
   (DECISIONS.md E154; a virtual base is one subobject however often it is
   reached).  */

static void
collect_overridden_functions (tree complete, tree binfo, tree fndecl,
			      vec<tree> &fns, vec<tree> &binfos)
{
  tree base_binfo;
  for (unsigned i = 0; BINFO_BASE_ITERATE (binfo, i, base_binfo); ++i)
    {
      if (tree fn = look_for_overrides_here (BINFO_TYPE (base_binfo), fndecl))
	{
	  tree sub = base_binfo;
	  if (BINFO_VIRTUAL_P (sub))
	    sub = binfo_for_vbase (BINFO_TYPE (sub), complete);
	  bool per_subobject = (fns.contains (fn)
				&& contracts_name_this_p (fn));
	  bool found = false;
	  for (unsigned j = 0; j < fns.length (); ++j)
	    if (fns[j] == fn && (!per_subobject || binfos[j] == sub))
	      found = true;
	  if (!found)
	    {
	      fns.safe_push (fn);
	      binfos.safe_push (sub);
	    }
	}
      collect_overridden_functions (complete, base_binfo, fndecl, fns,
				    binfos);
    }
}

/* An override pair of the function being finished: an overridden function
   and what its override shadow places.  */

struct override_pair
{
  /* B::f, and the base class (in D) it was first found in.  */
  tree base;
  tree binfo;
  /* The pair's number once registered, else 0.  */
  unsigned id;
  /* A marker of the pair was placed, so its regions at entry are needed;
     one of them for a postcondition of B::f.  */
  bool marked;
  bool base_post_marked;
  /* At entry: the assignments of the snapshots of `this' and of the
     parameters, and the markers of D::f's preconditions.  */
  tree snapshots;
  tree pre_obligations;
  /* After D::f's postconditions' checks: the assignment of the result's
     snapshot, if any, then the markers of B::f's postconditions.  */
  tree post_obligations;
  /* Copies of D::f's postconditions, for their markers, if any may be
     deferred.  */
  tree post_copies;
  /* The snapshot of `this' (a B *), of each parameter of D::f that needs
     one, and the result as B::f's postconditions see it.  */
  tree this_snapshot;
  hash_map<tree, tree> *parm_snapshots;
  tree result;
};

/* The override shadows of the function being finished.  */

struct override_shadows
{
  ~override_shadows ()
  {
    for (override_pair &p : pairs)
      delete p.parm_snapshots;
  }

  tree fndecl;
  /* Whether a strategy may defer the evaluations in FNDECL's body
     (contract_proof_deferral_ok_p), and whether the pairs' evaluations are
     made in override shadows (contract_proof_override_shadows_p): if not,
     each is an obligation of FNDECL itself, with no premise from the
     overridden function -- FNDECL's preconditions are decided at once,
     FNDECL's postconditions need no more than their own analysis
     evaluations, and the overridden functions' postconditions are proven
     in FNDECL's ordinary shadow.  */
  bool deferrable;
  bool shadows;
  auto_vec<override_pair> pairs;
  /* The snapshot variables, and the statement list of the block
     maybe_apply_function_contracts builds, which declares them and starts
     with the regions at entry (finish_override_shadows).  */
  auto_vec<tree> vars;
  tree list;
};

/* While maybe_apply_function_contracts runs, the override shadows of the
   function it finishes, if any (begin_override_shadows).  */

static override_shadows *current_override_shadows;

/* A new variable of TYPE in the body of OV's function, for a snapshot.  */

static tree
make_override_snapshot (override_shadows *ov, tree type)
{
  char name[32];
  sprintf (name, "__contract_override_%u", ov->vars.length ());
  tree var = build_decl (DECL_SOURCE_LOCATION (ov->fndecl), VAR_DECL,
			 get_identifier (name), cv_unqualified (type));
  DECL_ARTIFICIAL (var) = 1;
  DECL_IGNORED_P (var) = 1;
  TREE_USED (var) = 1;
  DECL_CONTEXT (var) = ov->fndecl;
  layout_decl (var, 0);
  ov->vars.safe_push (var);
  return var;
}

/* The region `if (.CONTRACT_SHADOW_GUARD (ID)) STMT;', executed only in
   override pair ID's shadow.  */

static tree
override_region (unsigned id, tree stmt)
{
  location_t loc = DECL_SOURCE_LOCATION (current_function_decl);
  contract_shadow_note_region ();
  tree guard = build_call_expr_internal_loc (loc, IFN_CONTRACT_SHADOW_GUARD,
					     boolean_type_node, 1,
					     build_int_cst (unsigned_type_node,
							    id));
  return build3_loc (loc, COND_EXPR, void_type_node, guard, stmt,
		     build_empty_stmt (loc));
}

/* STMT, code of OV's override shadows, as placed in OV's function: in a
   destructor with an in-charge parameter (of a class with virtual bases),
   for the complete-object destructor only, which is what a delete through
   a base class runs; the base-object destructor, which does not destroy
   the virtual bases, is never the target of a virtual call.  The clones
   fold the test.  */

static tree
override_in_charge (override_shadows *ov, tree stmt)
{
  if (!stmt || !DECL_HAS_IN_CHARGE_PARM_P (ov->fndecl))
    return stmt;
  return build_if_in_charge (stmt);
}

/* Register P, a pair of OV, if not yet done, and return its number.  */

static unsigned
register_override_pair (override_shadows *ov, override_pair *p)
{
  if (!p->id)
    p->id = contract_shadow_register_override (ov->fndecl, p->base);
  return p->id;
}

/* The kind of the regions of P, a pair of OV: the pair's number when it
   has an override shadow, and 0, the ordinary shadow's, otherwise.  */

static unsigned
override_kind (override_shadows *ov, override_pair *p)
{
  return ov->shadows ? register_override_pair (ov, p) : 0;
}

/* What PARM, a parameter of OV's function, is in P's B::f's
   postconditions: a snapshot taken at entry, unless D::f receives it by
   reference or by invisible reference (E153).  */

static tree
override_parm_snapshot (override_shadows *ov, override_pair *p, tree parm)
{
  tree type = TREE_TYPE (parm);
  if (TYPE_REF_P (type) || TREE_ADDRESSABLE (type))
    return parm;
  if (!p->parm_snapshots)
    p->parm_snapshots = new hash_map<tree, tree>;
  if (tree *snap = p->parm_snapshots->get (parm))
    return *snap;
  tree snap = make_override_snapshot (ov, type);
  p->parm_snapshots->put (parm, snap);
  append_to_statement_list_force (build2 (MODIFY_EXPR, TREE_TYPE (snap),
					  snap, parm),
				  &p->snapshots);
  return snap;
}

/* The snapshot of OV's function's `this' converted to P's base class,
   taken at entry, or error_mark_node.  */

static tree
override_this_snapshot (override_shadows *ov, override_pair *p)
{
  if (p->this_snapshot)
    return p->this_snapshot;
  tree conv = build_base_path (PLUS_EXPR, DECL_ARGUMENTS (ov->fndecl),
			       p->binfo, /*nonnull=*/1, tf_none);
  if (conv == error_mark_node)
    return p->this_snapshot = error_mark_node;
  tree snap = make_override_snapshot (ov, TREE_TYPE (conv));
  append_to_statement_list_force (build2 (MODIFY_EXPR, TREE_TYPE (snap),
					  snap, conv),
				  &p->snapshots);
  return p->this_snapshot = snap;
}

/* The result of OV's function as P's B::f's postconditions see it, or
   error_mark_node: for a covariant return, a snapshot of the result
   converted to B::f's return type -- null-checked for a pointer, as the
   covariant thunk converts it -- and otherwise the result itself, or, if
   ADDRESSABLE, the address of a postcondition's result binding being taken,
   a snapshot of it when it is not returned in memory, so that the binding
   has one home (postcondition_needs_retval_temp_p).  A snapshot is
   assigned in a region placed after D::f's postconditions' checks.  */

static tree
override_result (override_shadows *ov, override_pair *p, bool addressable)
{
  if (p->result)
    return p->result;
  tree fndecl = ov->fndecl;
  tree res = DECL_RESULT (fndecl);
  tree dtype = TREE_TYPE (TREE_TYPE (fndecl));
  tree btype = TREE_TYPE (TREE_TYPE (p->base));
  tree type = dtype;
  tree init = NULL_TREE;
  if (!res)
    return p->result = error_mark_node;
  if (!same_type_ignoring_top_level_qualifiers_p (dtype, btype))
    {
      bool ref = TYPE_REF_P (dtype);
      if (ref ? !TYPE_REF_P (btype) : !(TYPE_PTR_P (dtype)
					 && TYPE_PTR_P (btype)))
	return p->result = error_mark_node;
      tree binfo = lookup_base (TYPE_MAIN_VARIANT (TREE_TYPE (dtype)),
				TYPE_MAIN_VARIANT (TREE_TYPE (btype)),
				ba_unique, NULL, tf_none);
      if (!binfo || binfo == error_mark_node)
	return p->result = error_mark_node;
      tree ptr = (ref ? build_nop (build_pointer_type (TREE_TYPE (dtype)), res)
		  : res);
      init = build_base_path (PLUS_EXPR, ptr, binfo, /*nonnull=*/ref,
			      tf_none);
      if (init == error_mark_node)
	return p->result = error_mark_node;
      init = build_nop (btype, init);
      type = btype;
    }
  else if (addressable
	   && !aggregate_value_p (dtype, fndecl)
	   && trivially_copyable_p (dtype))
    init = res;
  if (!init)
    return p->result = res;
  tree snap = make_override_snapshot (ov, type);
  if (addressable)
    TREE_ADDRESSABLE (snap) = 1;
  append_to_statement_list_force
    (override_region (override_kind (ov, p),
		      build2 (INIT_EXPR, TREE_TYPE (snap), snap, init)),
     &p->post_obligations);
  return p->result = snap;
}

/* What a predicate of the overridden function becomes in the overriding
   function's override shadow: its parameters (PARMS, which also maps a
   postcondition's result binding) and `this' (THIS_EXPR, the overriding
   function's `this' converted to the base class).  For B::f's
   postconditions SNAPSHOTS is true: the parameters that need one map to
   P's snapshots, and `this' to P's (override_parm_snapshot,
   override_this_snapshot).  */

struct override_remap_data
{
  hash_map<tree, tree> *parms;
  tree this_expr;
  bool snapshots;
  override_shadows *ov;
  override_pair *p;
};

/* cp_walk_tree callback rewriting an unshared copy of a predicate of the
   overridden function (override_remap_data).  Stops at a parameter it
   cannot map and at anything unsharing does not copy or that declares or
   initializes an object of its own (a saved expression, a temporary, a
   statement, a lambda): such a predicate is not used.  */

static tree
override_remap_r (tree *tp, int *walk_subtrees, void *data_)
{
  override_remap_data *data = static_cast<override_remap_data *> (data_);
  tree t = *tp;
  if (TYPE_P (t))
    {
      *walk_subtrees = 0;
      return NULL_TREE;
    }
  switch (TREE_CODE (t))
    {
    case SAVE_EXPR:
    case TARGET_EXPR:
    case AGGR_INIT_EXPR:
    case BIND_EXPR:
    case STATEMENT_LIST:
    case DECL_EXPR:
    case STMT_EXPR:
    case LAMBDA_EXPR:
      return t;
    case PARM_DECL:
      *walk_subtrees = 0;
      /* The real `this' or the placeholder of a predicate parsed on a
	 declaration.  */
      if (is_this_parameter (t))
	{
	  tree e = (data->snapshots
		    ? override_this_snapshot (data->ov, data->p)
		    : unshare_expr (data->this_expr));
	  if (e == error_mark_node)
	    return t;
	  *tp = e;
	  return NULL_TREE;
	}
      if (tree *r = data->parms->get (t))
	{
	  tree e = *r;
	  if (data->snapshots && TREE_CODE (e) == PARM_DECL)
	    e = override_parm_snapshot (data->ov, data->p, e);
	  *tp = e;
	  return NULL_TREE;
	}
      return t;
    default:
      return NULL_TREE;
    }
}

/* Map in PARMS each parameter of BASE but `this' (and the in-charge and
   VTT parameters of a destructor) to the corresponding parameter of
   FNDECL.  */

static void
map_override_parms (tree base, tree fndecl, hash_map<tree, tree> &parms)
{
  for (tree bp = FUNCTION_FIRST_USER_PARM (base),
	 dp = FUNCTION_FIRST_USER_PARM (fndecl);
       bp && dp; bp = DECL_CHAIN (bp), dp = DECL_CHAIN (dp))
    parms.put (bp, dp);
}

/* True if CONTRACT, of FN, has a usable predicate and a prove or diagnose
   analysis evaluation.  */

static bool
override_obligation_p (tree contract, tree fn)
{
  return (CONTRACT_CONDITION (contract) != error_mark_node
	  && !CONTRACT_CONDITION_DEFERRED_P (contract)
	  && contract_semantic_proving_p (ensure_analysis_semantic (contract,
								    fn)));
}

/* Offer the obligation of CONTRACT, with the analysis semantic ANALYSIS
   and the predicate PRED as the override shadow of P, a pair of OV,
   evaluates it (OPAQUE if it cannot be described), to the strategies at
   OV's function's definition; without override shadows, as an obligation
   of OV's function.  If DEFER, a strategy may defer it: its marker is then
   appended to LIST.  Returns true if a marker was placed.  */

static bool
offer_override_obligation (override_shadows *ov, override_pair *p,
			   tree contract, contract_evaluation_semantic analysis,
			   tree pred, bool defer, bool opaque, tree *list)
{
  unsigned id = register_override_pair (ov, p);
  tree marker = NULL_TREE;
  diagnose_unproven_contract (contract, analysis,
			      DECL_SOURCE_LOCATION (ov->fndecl), NULL_TREE,
			      NULL_TREE, defer && !opaque ? &marker : NULL,
			      pred, opaque, id, ov->shadows);
  if (!marker)
    return false;
  append_to_statement_list_force (marker, list);
  p->marked = true;
  return true;
}

/* The obligations of P, a pair of OV, at entry: the preconditions of OV's
   function, each on a fresh copy.  */

static void
build_override_pre_obligations (override_shadows *ov, override_pair *p)
{
  tree fndecl = ov->fndecl;
  tree specs = get_fn_contract_specifiers (fndecl);
  if (!specs)
    return;
  tree copies = NULL_TREE;
  unsigned n = 0;
  for (tree c : tree_vec_range (specs))
    {
      if (!PRECONDITION_P (c))
	continue;
      if (!copies)
	copies = copy_contracts (fndecl, cmk_pre);
      tree copy = TREE_VEC_ELT (copies, n++);
      if (!override_obligation_p (c, fndecl)
	  || CONTRACT_CONDITION (copy) == error_mark_node
	  || CONTRACT_CONDITION_DEFERRED_P (copy))
	continue;
      remap_dummy_this (fndecl, &CONTRACT_CONDITION (copy));
      /* Without an override shadow, nothing is given: only folding can
	 settle it.  */
      offer_override_obligation (ov, p, copy,
				 ensure_analysis_semantic (c, fndecl),
				 CONTRACT_CONDITION (copy), ov->shadows,
				 false, &p->pre_obligations);
    }
}

/* The obligations of P, a pair of OV, for the postconditions of OV's
   function, made only in an override shadow: decided now for one with
   captures (contract_proof_deferrable_p), and otherwise placed ahead of
   their checks (add_override_post_obligations), for which this makes
   copies of them.  */

static void
build_override_post_obligations (override_shadows *ov, override_pair *p)
{
  tree fndecl = ov->fndecl;
  tree specs = get_fn_contract_specifiers (fndecl);
  /* Without override shadows, their own analysis evaluations are all
     there is.  */
  if (!specs || !ov->shadows)
    return;
  bool later = false;
  for (tree c : tree_vec_range (specs))
    if (POSTCONDITION_P (c) && override_obligation_p (c, fndecl))
      {
	if (!POSTCONDITION_CAPTURES (c))
	  later = true;
	else
	  /* Decided at once, as the postcondition's own analysis
	     evaluation is (contract_proof_deferrable_p).  */
	  offer_override_obligation (ov, p, c,
				     ensure_analysis_semantic (c, fndecl),
				     NULL_TREE, false, false, NULL);
      }
  if (later)
    p->post_copies = copy_contracts (fndecl, cmk_post);
}

/* The obligations of P, a pair of OV, for the postconditions of P's B::f,
   after OV's function's postconditions' checks: in P's post_obligations,
   their predicates rewritten with the snapshots and the result as the
   override shadow sees them.  */

static void
build_override_base_post_obligations (override_shadows *ov, override_pair *p)
{
  tree base = p->base;
  tree specs = get_fn_contract_specifiers (base);
  if (!specs)
    return;
  bool addressable = false;
  for (tree c : tree_vec_range (specs))
    if (POSTCONDITION_P (c))
      if (tree r = POSTCONDITION_IDENTIFIER (c))
	if (DECL_P (r) && TREE_ADDRESSABLE (r))
	  addressable = true;
  for (tree c : tree_vec_range (specs))
    {
      if (!POSTCONDITION_P (c) || !override_obligation_p (c, base))
	continue;
      contract_evaluation_semantic analysis
	= ensure_analysis_semantic (c, base);
      if (!ov->deferrable || POSTCONDITION_CAPTURES (c))
	{
	  /* Decided at once (contract_proof_deferrable_p): the captures
	     are initialized only in the wrapper.  */
	  offer_override_obligation (ov, p, c, analysis, NULL_TREE, false,
				     false, NULL);
	  continue;
	}
      hash_map<tree, tree> parms;
      map_override_parms (base, ov->fndecl, parms);
      bool opaque = false;
      tree r = POSTCONDITION_IDENTIFIER (c);
      if (r && r != error_mark_node && TREE_CODE (r) == PARM_DECL)
	{
	  tree result = override_result (ov, p, addressable);
	  if (result == error_mark_node)
	    opaque = true;
	  else
	    parms.put (r, result);
	}
      override_remap_data data = { &parms, NULL_TREE, true, ov, p };
      tree pred = unshare_expr (CONTRACT_CONDITION (c));
      if (!opaque && cp_walk_tree (&pred, override_remap_r, &data, NULL))
	opaque = true;
      if (offer_override_obligation (ov, p, c, analysis,
				     opaque ? CONTRACT_CONDITION (c) : pred,
				     true, opaque, &p->post_obligations))
	p->base_post_marked = true;
    }
}

/* D4405: the override shadows of FNDECL, the function being finished, or
   null if there are none to place: per overridden function with at least
   one obligation, FNDECL's preconditions' obligations and that function's
   postconditions' are built now, FNDECL's postconditions' as their checks
   are (add_override_post_obligations).  Sets current_override_shadows.
   Called from maybe_apply_function_contracts, so FNDECL is the current
   function.  */

override_shadows *
begin_override_shadows (tree fndecl)
{
  if (!flag_contracts
      || !flag_contracts_p4405
      || !flag_contracts_p3097
      || flag_syntax_only
      || processing_template_decl
      || !DECL_VIRTUAL_P (fndecl)
      /* Never the final overrider of a call through a base class.  */
      || DECL_PURE_VIRTUAL_P (fndecl)
      || DECL_CLONED_FUNCTION_P (fndecl)
      || CONTRACT_HELPER (fndecl) != ldf_contract_none
      || DECL_IS_WRAPPER_FN_P (fndecl))
    return NULL;

  /* FNDECL's own obligations, in every pair.  */
  bool own = false;
  if (tree specs = get_fn_contract_specifiers (fndecl))
    for (tree c : tree_vec_range (specs))
      if (override_obligation_p (c, fndecl))
	own = true;

  auto_vec<tree> bases, binfos;
  tree ctx = DECL_CONTEXT (fndecl);
  collect_overridden_functions (ctx, TYPE_BINFO (ctx), fndecl, bases, binfos);
  if (bases.is_empty ())
    return NULL;

  override_shadows *ov = new override_shadows;
  ov->fndecl = fndecl;
  ov->deferrable = contract_proof_deferral_ok_p (fndecl);
  ov->shadows = ov->deferrable && contract_proof_override_shadows_p (fndecl);
  bool any = false;
  for (unsigned i = 0; i < bases.length (); ++i)
    {
      tree base = bases[i];
      /* A pure virtual function or an uninstantiated template has had
	 nothing substitute its contracts yet, as for a wrapper.  */
      maybe_instantiate_contracts (base, /*for_wrapper=*/true);
      tree base_specs = get_fn_contract_specifiers (base);
      if (base_specs && contract_any_deferred_p (base_specs))
	continue;
      bool base_post = false;
      if (base_specs)
	for (tree c : tree_vec_range (base_specs))
	  if (POSTCONDITION_P (c) && override_obligation_p (c, base))
	    base_post = true;
      if (!own && !base_post)
	continue;

      override_pair pair = {};
      pair.base = base;
      pair.binfo = binfos[i];
      ov->pairs.safe_push (pair);
      override_pair *p = &ov->pairs.last ();
      build_override_pre_obligations (ov, p);
      build_override_post_obligations (ov, p);
      build_override_base_post_obligations (ov, p);
      if (p->marked || p->post_copies)
	any = true;
    }
  if (!any)
    {
      delete ov;
      return NULL;
    }
  current_override_shadows = ov;
  return ov;
}

/* True if OV has a marker for a postcondition of an overridden function,
   which needs the try/finally of a postcondition check.  */

static bool
override_shadows_post_p (override_shadows *ov)
{
  for (override_pair &p : ov->pairs)
    if (p.base_post_marked)
      return true;
  return false;
}

/* D4405: in apply_postconditions, ahead of the check of POST, a
   postcondition of the current function, or of every one if POST is null,
   its obligations in the current function's override shadows.  */

void
add_override_post_obligations (tree post)
{
  override_shadows *ov = current_override_shadows;
  if (!ov)
    return;
  tree fndecl = ov->fndecl;
  tree specs = get_fn_contract_specifiers (fndecl);
  for (override_pair &p : ov->pairs)
    {
      if (!p.post_copies)
	continue;
      unsigned n = 0;
      for (tree c : tree_vec_range (specs))
	{
	  if (!POSTCONDITION_P (c))
	    continue;
	  tree copy = TREE_VEC_ELT (p.post_copies, n++);
	  if ((post && c != post)
	      || POSTCONDITION_CAPTURES (c)
	      || !override_obligation_p (c, fndecl)
	      || CONTRACT_CONDITION (copy) == error_mark_node
	      || CONTRACT_CONDITION_DEFERRED_P (copy))
	    continue;
	  remap_dummy_this (fndecl, &CONTRACT_CONDITION (copy));
	  if (CONTRACT_CONDITION (copy) == error_mark_node)
	    continue;
	  /* The result, read as the check reads it.  */
	  remap_retval (fndecl, copy);
	  tree list = NULL_TREE;
	  if (offer_override_obligation (ov, &p, copy,
					 ensure_analysis_semantic (c, fndecl),
					 CONTRACT_CONDITION (copy), true, false,
					 &list))
	    add_stmt (override_in_charge (ov, list));
	}
    }
}

/* The regions at entry of OV's override shadows: per pair that placed a
   marker, the snapshots, and, in an override shadow, the overridden
   function's preconditions as premises -- its parameters mapped by
   position and its `this' to the base class subobject, as the virtual
   call's wrapper converts it -- and the obligations of OV's function's
   preconditions.  */

static tree
override_entry_regions (override_shadows *ov)
{
  tree fndecl = ov->fndecl;
  tree regions = NULL_TREE;
  for (override_pair &p : ov->pairs)
    {
      if (!p.marked)
	continue;
      if (p.base_post_marked && p.snapshots)
	append_to_statement_list_force (override_region (override_kind (ov,
									&p),
							 p.snapshots),
					&regions);
      /* Without an override shadow, nothing is given.  */
      if (!ov->shadows)
	continue;
      /* Guaranteed only when their labels settle them (DECISIONS.md E151):
	 they are evaluated by the callers, in any translation unit.  */
      tree base_specs = get_fn_contract_specifiers (p.base);
      hash_map<tree, tree> parms;
      map_override_parms (p.base, fndecl, parms);
      override_remap_data data
	= { &parms, build_base_path (PLUS_EXPR, DECL_ARGUMENTS (fndecl),
				     p.binfo, /*nonnull=*/1, tf_none),
	    false, ov, &p };
      if (base_specs && data.this_expr != error_mark_node)
	for (tree c : tree_vec_range (base_specs))
	  {
	    if (!PRECONDITION_P (c)
		|| CONTRACT_CONDITION (c) == error_mark_node)
	      continue;
	    ensure_evaluation_semantic (c, p.base, /*in_ce=*/false);
	    tree pred = unshare_expr (CONTRACT_CONDITION (c));
	    if (cp_walk_tree (&pred, override_remap_r, &data, NULL))
	      continue;
	    bool guaranteed = contract_premise_settled_p (c);
	    /* As the premise of an assume evaluation, or of an observe one,
	       which may not hold.  */
	    if (tree a = contract_proof_assumption (pred,
						    guaranteed ? CES_ASSUME
						    : CES_OBSERVE,
						    EXPR_LOCATION (c), p.id))
	      append_to_statement_list_force (a, &regions);
	  }
      append_to_statement_list_force (p.pre_obligations, &regions);
    }
  return override_in_charge (ov, regions);
}

/* Put REGIONS (override_entry_regions) at the start of the body of FNDECL,
   which maybe_apply_function_contracts adds nothing else to: inside the
   MUST_NOT_THROW_EXPR of a noexcept function, as preconditions are.  */

static void
prepend_override_shadow_regions (tree fndecl, tree regions)
{
  tree *body = &DECL_SAVED_TREE (fndecl);
  tree first = (TYPE_NOEXCEPT_P (TREE_TYPE (fndecl))
		? expr_first (*body) : NULL_TREE);
  if (first && first != error_mark_node
      && TREE_CODE (first) == MUST_NOT_THROW_EXPR)
    body = &TREE_OPERAND (first, 0);
  if (TREE_CODE (*body) == STATEMENT_LIST)
    {
      /* The function body is the statement list still being built.  */
      tree_stmt_iterator i = tsi_start (*body);
      tsi_link_before (&i, regions, TSI_SAME_STMT);
    }
  else
    {
      tree list = NULL_TREE;
      append_to_statement_list_force (regions, &list);
      append_to_statement_list_force (*body, &list);
      *body = list;
    }
}

/* Be done with OV, the override shadows of the function being finished.  */

static void
end_override_shadows (override_shadows *ov)
{
  if (current_override_shadows == ov)
    current_override_shadows = NULL;
  delete ov;
}

/* Place the regions at entry of OV's override shadows
   (override_entry_regions) at the start of OV's statement list, declare the
   snapshots there, and be done with OV.  */

static void
finish_override_shadows (override_shadows *ov)
{
  if (tree regions = override_entry_regions (ov))
    {
      tree_stmt_iterator i = tsi_start (ov->list);
      tsi_link_before (&i, regions, TSI_SAME_STMT);
    }
  for (tree var : ov->vars)
    pushdecl (var);
  end_override_shadows (ov);
}

/* D4405: in maybe_apply_function_contracts, LIST is the statement list of
   the block it builds around the body of OV's function, whose
   postconditions' checks the block ends with.  */

void
start_override_shadows (override_shadows *ov, tree list)
{
  ov->list = list;
}

/* D4405: after the current function's postconditions' checks, the
   obligations of the overridden functions' postconditions in its override
   shadows; then its regions at entry (finish_override_shadows).  */

void
add_override_base_post_obligations ()
{
  override_shadows *ov = current_override_shadows;
  if (!ov)
    return;
  for (override_pair &p : ov->pairs)
    if (p.base_post_marked)
      add_stmt (override_in_charge (ov, p.post_obligations));
  finish_override_shadows (ov);
}

/* D4405: place OV's override shadows in the body of their function, which
   has no active postconditions (maybe_apply_function_contracts): the
   regions at entry alone when there is nothing else to add, and otherwise
   in a block, as maybe_apply_function_contracts builds one -- the regions
   at entry and the function's own active preconditions, then the body, in
   a try/finally whose normal exit has the overridden functions'
   postconditions' obligations if any needs a marker.  The real function
   sheds the regions before SSA, and with them what the try/finally
   holds.  */

void
place_override_shadows_alone (override_shadows *ov)
{
  tree fndecl = ov->fndecl;
  bool pre = handle_contracts_p (fndecl) && has_active_preconditions (fndecl);
  bool post = override_shadows_post_p (ov);
  if (!pre && !post)
    {
      if (tree regions = override_entry_regions (ov))
	prepend_override_shadow_regions (fndecl, regions);
      end_override_shadows (ov);
      return;
    }

  /* As maybe_apply_function_contracts: inside the MUST_NOT_THROW_EXPR of a
     noexcept function, if it has one.  */
  tree fnbody;
  tree m_n_t_expr = (TYPE_NOEXCEPT_P (TREE_TYPE (fndecl))
		     ? expr_first (DECL_SAVED_TREE (fndecl)) : NULL_TREE);
  if (m_n_t_expr && m_n_t_expr != error_mark_node
      && TREE_CODE (m_n_t_expr) == MUST_NOT_THROW_EXPR)
    {
      fnbody = TREE_OPERAND (m_n_t_expr, 0);
      TREE_OPERAND (m_n_t_expr, 0) = push_stmt_list ();
    }
  else
    {
      fnbody = DECL_SAVED_TREE (fndecl);
      DECL_SAVED_TREE (fndecl) = push_stmt_list ();
    }

  tree compound_stmt = begin_compound_stmt (0);
  current_binding_level->artificial = true;
  start_override_shadows (ov, cur_stmt_list);
  if (pre)
    apply_preconditions (fndecl);
  if (!post)
    {
      add_stmt (fnbody);
      finish_override_shadows (ov);
      finish_compound_stmt (compound_stmt);
      return;
    }
  tree try_fin = build_stmt (UNKNOWN_LOCATION, TRY_FINALLY_EXPR, fnbody,
			     NULL_TREE);
  add_stmt (try_fin);
  TREE_OPERAND (try_fin, 1) = push_stmt_list ();
  if (flag_exceptions && !type_noexcept_p (TREE_TYPE (fndecl)))
    {
      /* Nothing on the exceptional path.  */
      tree eh_else = build_stmt (UNKNOWN_LOCATION, EH_ELSE_EXPR, NULL_TREE,
				 NULL_TREE);
      add_stmt (eh_else);
      TREE_OPERAND (eh_else, 0) = push_stmt_list ();
      add_override_base_post_obligations ();
      TREE_OPERAND (eh_else, 0) = pop_stmt_list (TREE_OPERAND (eh_else, 0));
      TREE_OPERAND (eh_else, 1) = push_stmt_list ();
      TREE_OPERAND (eh_else, 1) = pop_stmt_list (TREE_OPERAND (eh_else, 1));
    }
  else
    add_override_base_post_obligations ();
  TREE_OPERAND (try_fin, 1) = pop_stmt_list (TREE_OPERAND (try_fin, 1));
  finish_compound_stmt (compound_stmt);
}

#include "gt-cp-contracts.h"
