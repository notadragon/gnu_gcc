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
#include "contracts-config.h"
#include "cp-objcp-common.h"
#include "tree.h"
#include "tree-inline.h"
#include "attribs.h"
#include "tree-iterator.h"
#include "print-tree.h"
#include "stor-layout.h"
#include "intl.h"
#include "cgraph.h"
#include "opts.h"
#include "output.h"
#include "varasm.h"
#include "ubsan.h"

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
  This is done during `cp_genericize ()`.  In the current implementation, the
  decision on the control flow is made on the basis of the setting of a command-
  line flag that determines a TU-wide contract evaluation semantic, which has
  the following initial set of behaviours:

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

  /* Compare the conditions of the contracts.  */
  tree t1 = cp_fully_fold_init (CONTRACT_CONDITION (old_contract));
  tree t2 = cp_fully_fold_init (CONTRACT_CONDITION (new_contract));

  /* Compare the contracts. */

  bool saved_comparing_contracts = comparing_contracts;
  comparing_contracts = true;
  bool matching_p = cp_tree_equal (t1, t2);
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
  if ((old_msg == NULL_TREE) != (new_msg == NULL_TREE)
      || (old_msg && new_msg
	  && (TREE_STRING_LENGTH (old_msg) != TREE_STRING_LENGTH (new_msg)
	      || memcmp (TREE_STRING_POINTER (old_msg),
			 TREE_STRING_POINTER (new_msg),
			 TREE_STRING_LENGTH (old_msg)) != 0)))
    {
      auto_diagnostic_group d;
      error_at (EXPR_LOCATION (new_contract),
		"mismatched contract diagnostic message in declaration");
      inform (EXPR_LOCATION (old_contract), "previous contract here");
      return true;
    }

  /* Compare assertion-control labels (P3400).  Both must specify the same
     label, or neither specifies one.  */
  tree old_label = CONTRACT_LABEL (old_contract);
  tree new_label = CONTRACT_LABEL (new_contract);
  if ((old_label == NULL_TREE) != (new_label == NULL_TREE))
    {
      auto_diagnostic_group d;
      error_at (EXPR_LOCATION (new_contract),
		"mismatched assertion-control label in declaration");
      inform (EXPR_LOCATION (old_contract), "previous contract here");
      return true;
    }
  if (old_label && new_label)
    {
      tree l1 = cp_fully_fold_init (old_label);
      tree l2 = cp_fully_fold_init (new_label);
      if (!cp_tree_equal (l1, l2))
	{
	  auto_diagnostic_group d;
	  error_at (EXPR_LOCATION (new_contract),
		    "mismatched assertion-control label in declaration");
	  inform (EXPR_LOCATION (old_contract), "previous contract here");
	  return true;
	}
    }

  /* Compare postcondition captures (P3098).  */
  if (TREE_CODE (old_contract) == POSTCONDITION_STMT)
    {
      tree old_caps = POSTCONDITION_CAPTURES (old_contract);
      tree new_caps = POSTCONDITION_CAPTURES (new_contract);

      /* Both must have captures or neither.  */
      if ((old_caps == NULL_TREE) != (new_caps == NULL_TREE))
	{
	  auto_diagnostic_group d;
	  error_at (EXPR_LOCATION (new_contract),
		    "mismatched postcondition captures in declaration");
	  inform (EXPR_LOCATION (old_contract), "previous contract here");
	  return true;
	}

      /* Compare captures pairwise: same count, same names, same inits.  */
      if (old_caps && new_caps)
	{
	  tree oc = old_caps, nc = new_caps;
	  bool saved_cc = comparing_contracts;
	  comparing_contracts = true;
	  for (; oc && nc; oc = TREE_CHAIN (oc), nc = TREE_CHAIN (nc))
	    {
	      tree old_var = TREE_VALUE (oc);
	      tree new_var = TREE_VALUE (nc);
	      if (DECL_NAME (old_var) != DECL_NAME (new_var)
		  || !cp_tree_equal (DECL_INITIAL (old_var),
				     DECL_INITIAL (new_var)))
		{
		  comparing_contracts = saved_cc;
		  auto_diagnostic_group d;
		  error_at (EXPR_LOCATION (new_contract),
			    "mismatched postcondition captures in declaration");
		  inform (EXPR_LOCATION (old_contract),
			  "previous contract here");
		  return true;
		}
	    }
	  comparing_contracts = saved_cc;
	  if (oc || nc)
	    {
	      auto_diagnostic_group d;
	      error_at (EXPR_LOCATION (new_contract),
			"mismatched postcondition captures in declaration");
	      inform (EXPR_LOCATION (old_contract), "previous contract here");
	      return true;
	    }
	}
    }

  /* Compare requires-clauses (P4283).  Both must specify the same
     constraint, or neither specifies one.  */
  tree old_req = CONTRACT_REQUIRES_CLAUSE (old_contract);
  tree new_req = CONTRACT_REQUIRES_CLAUSE (new_contract);
  if ((old_req == NULL_TREE) != (new_req == NULL_TREE))
    {
      auto_diagnostic_group d;
      error_at (EXPR_LOCATION (new_contract),
		"mismatched requires clause on contract assertion "
		"in declaration");
      inform (EXPR_LOCATION (old_contract), "previous contract here");
      return true;
    }
  if (old_req && new_req)
    {
      bool saved_cc = comparing_contracts;
      comparing_contracts = true;
      bool match = cp_tree_equal (old_req, new_req);
      comparing_contracts = saved_cc;
      if (!match)
	{
	  auto_diagnostic_group d;
	  error_at (EXPR_LOCATION (new_contract),
		    "mismatched requires clause on contract assertion "
		    "in declaration");
	  inform (EXPR_LOCATION (old_contract), "previous contract here");
	  return true;
	}
    }

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

/* True if SEM causes no contract check to be emitted.  For now the P3100
   "assume" semantic behaves identically to "ignore" in codegen, so both
   are treated the same way at every code-generation decision point.  */

static inline bool
contract_semantic_emits_no_check (unsigned sem)
{
  return sem == CES_IGNORE || sem == CES_ASSUME;
}

/* True if SEM never permits an exception to escape a handler call: the
   contract is either unchecked, or checked through a terminating entry
   point.  Used to decide whether a compiler-synthesized wrapper function
   (P3097, P3098) can be marked noexcept.  */

static inline bool
contract_semantic_is_nonthrowing (unsigned sem)
{
  return sem == CES_IGNORE || sem == CES_QUICK || sem == CES_ASSUME
	 || sem == CES_NOEXCEPT_ENFORCE || sem == CES_NOEXCEPT_OBSERVE;
}

/* True if every contract in CONTRACTS -- the TREE_VEC of contract-specifier
   *_STMT nodes returned by get_fn_contract_specifiers -- has a
   statically fixed, nonthrowing evaluation semantic: no P3595 dynamic
   descriptor (CONTRACT_DYNAMIC) and no assertion-control label
   (CONTRACT_LABEL) that could select a different (possibly throwing)
   semantic at runtime.  FNDECL is the context passed through to
   ensure_evaluation_semantic, matching how every other caller in this file
   resolves a contract's callee-side semantic.

   If ONLY_KIND is PRECONDITION_STMT or POSTCONDITION_STMT, contracts of the
   other kind are skipped -- used by build_contract_condition_function,
   whose outlined pre/post functions each only check one kind (see
   remap_and_emit_conditions).  ERROR_MARK (the default) means "every
   contract in the list", matching a wrapper's already-scoped contract
   list (see copy_and_remap_contracts's cmk_all/cmk_pre selection).

   ensure_evaluation_semantic must be called (to force resolution) before
   CONTRACT_DYNAMIC is inspected: that field is populated as a side effect
   of resolution (see contract_active_p above), so reading it beforehand
   would silently miss dynamic contracts whose semantic has not yet been
   resolved on this path.  In practice, by the time this is called from
   build_contract_condition_function or the wrapper builder, every contract
   here has already gone through contract_active_p (via
   contract_any_active_p), so this is a cache hit; the explicit call keeps
   the function correct even if that invariant ever changes.

   Conservatively returns false for anything it can't prove.  */

static bool
all_contracts_statically_nonthrowing (tree contracts, tree fndecl,
				      tree_code only_kind = ERROR_MARK)
{
  if (!contracts)
    return true;

  for (tree contract : tree_vec_range (contracts))
    {
      if (only_kind != ERROR_MARK && TREE_CODE (contract) != only_kind)
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

/* Return true if CONTRACT is checked or assumed under the current build
   configuration.  Returns true if EITHER the runtime or the constexpr
   evaluation semantic is non-ignore, since either can require the
   contract to be present in the function body.  */

static bool
contract_active_p (tree contract, tree fndecl)
{
  /* Resolve the runtime semantic first; this also caches the P3595
     dynamic-selector descriptor (if any) on the contract node.  */
  bool runtime_active = !contract_semantic_emits_no_check
			  (ensure_evaluation_semantic (contract, fndecl, false));

  /* A dynamic contract is always active at run time regardless of its
     compile-time default: the selector may return a checking semantic even
     when the default is "ignore" (P3595).  */
  if (CONTRACT_DYNAMIC (contract))
    runtime_active = true;

  return runtime_active
    || !contract_semantic_emits_no_check
	   (ensure_evaluation_semantic (contract, fndecl, true));
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
   particular call site is decided per call site in maybe_contract_wrap_call.  */

static bool
handle_caller_contracts_p (tree fndecl)
{
  return (flag_contracts
	  && !processing_template_decl
	  && (CONTRACT_HELPER (fndecl) == ldf_contract_none)
	  && DECL_HAS_CONTRACTS_P (fndecl));
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

/* Call LABEL.METHOD_ID(SEM_VAL) and constant-evaluate the result.
   Returns NULL_TREE if the method does not exist or evaluation fails.
   Used by ensure_evaluation_semantic (compute_semantic facet) and
   grok_contract (compute_comment / apply_label_string_facet).  */

static tree
call_label_method (tree label, tree method_id, uint16_t sem_val)
{
  if (!label || label == error_mark_node
      || !TREE_TYPE (label) || !CLASS_TYPE_P (TREE_TYPE (label))
      || type_dependent_expression_p (label))
    return NULL_TREE;
  tree label_type = TREE_TYPE (label);
  tree fn = lookup_member (label_type, method_id,
			   /*protect=*/0, /*want_type=*/false, tf_none);
  if (!fn || fn == error_mark_node)
    return NULL_TREE;
  tree fn_decl = (TREE_CODE (fn) == BASELINK
		  ? BASELINK_FUNCTIONS (fn) : fn);
  if (TREE_CODE (fn_decl) == OVERLOAD)
    fn_decl = OVL_FIRST (fn_decl);
  tree parm = FUNCTION_FIRST_USER_PARMTYPE (fn_decl);
  tree sem_type = parm ? TREE_VALUE (parm) : uint16_type_node;
  tree sem_arg = build_int_cst (sem_type, sem_val);
  vec<tree, va_gc> *args = NULL;
  vec_safe_push (args, sem_arg);
  tree call = build_new_method_call (label, fn, &args,
				     NULL_TREE, LOOKUP_NORMAL, NULL, tf_none);
  if (!call || call == error_mark_node)
    return NULL_TREE;
  return cxx_constant_value (call, NULL_TREE, tf_none);
}

/* Read the cached runtime callee-side semantic.  Valid only after
   ensure_evaluation_semantic(contract, fndecl, false) has been called.  */

contract_evaluation_semantic
get_evaluation_semantic (const_tree contract)
{
  tree s = CONTRACT_EVALUATION_SEMANTIC (contract);
  gcc_checking_assert (s != NULL_TREE);
  return (contract_evaluation_semantic) tree_to_uhwi (s);
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

/* Reconstruct a base contract_query from the stored AST fields.
   Caller must set caller_side and in_constant_evaluation before use.  */

/* The gated base set: the four C++26 semantics, plus P3100 "assume" only
   when -fcontracts-allow-assume is in effect.  make_contract_query intersects
   the (flag-independent) label restriction with this set, so "assume" can
   never be present -- from a label or otherwise -- when the flag is off.  */

static inline uint16_t
contract_base_allowed_mask ()
{
  uint16_t mask = CES_ALL_ALLOWED;
  if (flag_contracts_allow_assume)
    mask |= (1 << CES_ASSUME);
  if (flag_contracts_p4298)
    mask |= (1 << CES_NOEXCEPT_ENFORCE) | (1 << CES_NOEXCEPT_OBSERVE);
  return mask;
}

static contract_query
make_contract_query (tree contract, tree fndecl)
{
  contract_query q;
  q.fndecl = fndecl;
  q.caller_fndecl = NULL_TREE;
  q.loc = EXPR_LOCATION (contract);
  q.caller_loc = UNKNOWN_LOCATION;

  if (TREE_CODE (contract) == PRECONDITION_STMT)
    q.kind = CAK_PRE;
  else if (TREE_CODE (contract) == POSTCONDITION_STMT)
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

   The allowed set is the four C++26 semantics plus "assume" always -- implicit
   "assume" is intentionally NOT gated on -fcontracts-allow-assume, since it
   introduces no new UB (it is the status quo) -- plus the P4298 noexcept-
   terminating variants when -fcontracts-p4298 is in effect (matching how
   explicit contracts gate those).  */

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
     semantic, so only widen with them when ALLOWED already permits some checking
     semantic -- otherwise a check that supports no checking at all (e.g. an
     opaque [[assume]], ALLOWED == {ignore}) would have a handler semantic
     re-admitted under -fcontracts-p4298 and evaluate a predicate it must not.  */
  uint16_t mask = allowed | (1 << CES_ASSUME);
  const uint16_t checking_mask
    = (1 << CES_OBSERVE) | (1 << CES_ENFORCE) | (1 << CES_QUICK);
  if (flag_contracts_p4298 && (allowed & checking_mask))
    mask |= (1 << CES_NOEXCEPT_ENFORCE) | (1 << CES_NOEXCEPT_OBSERVE);

  contract_query q;
  q.fndecl = fndecl;
  q.caller_fndecl = NULL_TREE;
  q.kind = CAK_IMPLICIT;
  q.caller_side = false;
  q.in_constant_evaluation = false;
  q.allowed_mask = mask;
  q.groups = &groups;
  q.loc = loc;
  q.caller_loc = UNKNOWN_LOCATION;

  contract_config_result r = contract_config_resolve (&q);

  /* CES_INVALID means the allowed set admitted no semantic (e.g. a label whose
     allowed_semantics facet excludes everything the check supports).  That is an
     ill-formed configuration -- diagnose it rather than silently picking one.  */
  if (r.semantic == CES_INVALID)
    {
      error_at (loc, "no allowed evaluation semantic for the implicit contract "
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

struct implicit_ub_info {
  uint16_t allowed;
  bool defined_eb;
  const char *comment;
};

static bool
implicit_ub_group_info (const char *group, implicit_ub_info *out)
{
  static const uint16_t MID_END_ALLOWED
    = CES_ALL_ALLOWED & ~((1 << CES_ENFORCE) | (1 << CES_OBSERVE));
  if (strcmp (group, "ub:expr.unary.dereference.nullptr") == 0)
    {
      *out = { MID_END_ALLOWED, false, "null pointer dereference" };
      return true;
    }
  if (strcmp (group, "ub:expr.expr.eval.signed.integer") == 0)
    {
      *out = { MID_END_ALLOWED, true, "signed integer overflow" };
      return true;
    }
  if (strcmp (group, "ub:conv.lval.valid.representation.bool.enum") == 0)
    {
      *out = { MID_END_ALLOWED, true, "invalid value for its type" };
      return true;
    }
  if (strcmp (group, "ub:basic.align.object.alignment") == 0)
    {
      /* A misaligned access is an lvalue with no defined substitute, so ignore
	 is the raw access (like null-deref), not a coerced value.  */
      *out = { MID_END_ALLOWED, false, "misaligned pointer access" };
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
    case CES_IGNORE:
      /* Produce the defined erroneous value where one exists (e.g. the wrapped
	 result for signed overflow); otherwise no instrumentation.  */
      return info.defined_eb ? IMPLICIT_UB_DEFINED : IMPLICIT_UB_NONE;
    default:
      /* assume (and any unreachable throwing enf/obs): no instrumentation.  */
      return IMPLICIT_UB_NONE;
    }
}

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
  if (!gn_member || gn_member == error_mark_node)
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

  tree gn_folded = cxx_constant_value (gn_val, NULL_TREE, tf_none);
  if (!gn_folded || gn_folded == error_mark_node
      || TREE_CODE (gn_folded) != CONSTRUCTOR)
    {
      CONTRACT_GROUPS (contract) = error_mark_node;
      return;
    }

  tree groups_list = NULL_TREE;
  unsigned ix;
  tree val;
  FOR_EACH_CONSTRUCTOR_VALUE (CONSTRUCTOR_ELTS (gn_folded), ix, val)
    {
      const char *str = NULL;
      char *buf = NULL;
      if (TREE_CODE (val) == STRING_CST)
	str = TREE_STRING_POINTER (val);
      else if (TREE_CODE (val) == CONSTRUCTOR)
	{
	  unsigned len = CONSTRUCTOR_NELTS (val);
	  buf = XNEWVEC (char, len + 1);
	  unsigned k;
	  tree ch;
	  FOR_EACH_CONSTRUCTOR_VALUE (CONSTRUCTOR_ELTS (val), k, ch)
	    {
	      if (TREE_CODE (ch) == INTEGER_CST)
		buf[k] = (char) tree_to_uhwi (ch);
	      else
		buf[k] = '\0';
	    }
	  buf[len] = '\0';
	  str = buf;
	}
      if (str && str[0] != '\0')
	{
	  size_t slen = strlen (str);
	  groups_list = tree_cons (NULL_TREE,
				   build_string (slen + 1, str),
				   groups_list);
	}
      XDELETEVEC (buf);
    }

  CONTRACT_GROUPS (contract) = groups_list
    ? nreverse (groups_list) : error_mark_node;
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
   dispatch path to decide whether the semantic map is the identity.  */

static bool
label_has_compute_semantic (tree label)
{
  if (!label || label == error_mark_node
      || !TREE_TYPE (label) || !CLASS_TYPE_P (TREE_TYPE (label)))
    return false;
  tree fn = lookup_member (TREE_TYPE (label),
			   get_identifier ("compute_semantic"),
			   /*protect=*/0, /*want_type=*/false, tf_none);
  return fn && fn != error_mark_node;
}

/* Core of the compute_semantic P3400 facet: apply LABEL's compute_semantic to
   SEM, if present.  Sets *IN_ALLOWED to whether the (possibly transformed)
   result lies within ALLOWED_MASK.  Returns the raw computed value when the
   facet is present, else SEM unchanged (with *IN_ALLOWED true).  */

static uint16_t
compute_semantic_core (tree label, uint16_t sem, uint16_t allowed_mask,
		       bool *in_allowed)
{
  *in_allowed = true;
  if (!label || label == error_mark_node)
    return sem;
  tree cs_result = call_label_method (label,
				      get_identifier ("compute_semantic"),
				      sem);
  if (cs_result && TREE_CODE (cs_result) == INTEGER_CST)
    {
      uint16_t computed = (uint16_t) tree_to_uhwi (cs_result);
      *in_allowed = (allowed_mask & (1 << computed)) != 0;
      return computed;
    }
  return sem;
}

/* Apply the compute_semantic P3400 facet to SEM, if present on LABEL.
   Returns the (possibly transformed) semantic.  A result outside the allowed
   set is a compile-time error (the compile-time-resolved path).  */

static uint16_t
apply_compute_semantic (tree label, uint16_t sem, uint16_t allowed_mask,
			location_t loc)
{
  bool in_allowed;
  uint16_t computed = compute_semantic_core (label, sem, allowed_mask,
					     &in_allowed);
  if (in_allowed)
    return computed;
  /* A compute_semantic result outside the allowed set is an error --
     including "assume" when -fcontracts-allow-assume was not given, since
     the flag gate keeps assume out of the set entirely.  */
  error_at (loc, "%<compute_semantic%> result is not in the "
	    "allowed evaluation semantics");
  return sem;
}

/* Like apply_compute_semantic, but for the P3595 dynamic-dispatch path: when
   the compute_semantic result lands outside the allowed set, return
   CES_INVALID (the sentinel that stage 2 turns into a runtime enforced
   violation) instead of issuing a compile-time error.  */

static uint16_t
apply_compute_semantic_value (tree label, uint16_t sem, uint16_t allowed_mask)
{
  bool in_allowed;
  uint16_t computed = compute_semantic_core (label, sem, allowed_mask,
					     &in_allowed);
  return in_allowed ? computed : (uint16_t) CES_INVALID;
}

/* Lazily resolve the callee-side semantic for CONTRACT.  Uses the runtime
   slot when !IN_CE, the constexpr slot when IN_CE.  Caches the result.  */

contract_evaluation_semantic
ensure_evaluation_semantic (tree contract, tree fndecl, bool in_ce)
{
  tree *slot = in_ce
    ? &CONTRACT_CONSTEXPR_EVALUATION_SEMANTIC (contract)
    : &CONTRACT_EVALUATION_SEMANTIC (contract);

  if (*slot != NULL_TREE)
    return (contract_evaluation_semantic) tree_to_uhwi (*slot);

  contract_query q = make_contract_query (contract, fndecl);
  q.caller_side = false;
  q.in_constant_evaluation = in_ce;

  auto_vec<const char *> groups_vec;
  fill_query_groups (&q, contract, groups_vec);

  contract_config_result res = contract_config_resolve (&q);
  uint16_t sem = (uint16_t) res.semantic;
  bool dynamic_no_default = (!in_ce && res.dyn_name && res.no_static_default);
  if (sem == CES_INVALID && !dynamic_no_default)
    {
      error_at (EXPR_LOCATION (contract),
		"no valid evaluation semantic for contract assertion");
      sem = in_ce ? CES_OBSERVE : CES_ENFORCE;
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
      sem = CES_IGNORE;
    }
  else
    sem = apply_compute_semantic (CONTRACT_LABEL (contract), sem,
				  q.allowed_mask, EXPR_LOCATION (contract));

  *slot = build_int_cst (uint16_type_node, sem);

  /* Cache the runtime dynamic-selector descriptor, if any.  A dynamic
     descriptor only exists for the runtime slot (!in_ce); constant
     evaluation never has one (P3595 spec 3, enforced in
     contract_config_resolve).  Store the name as an IDENTIFIER_NODE and
     pack linkage/provideweak into an INTEGER_CST so the whole thing is
     GC-safe.  */
  if (!in_ce && res.dyn_name)
    {
      unsigned HOST_WIDE_INT packed
	= ((unsigned HOST_WIDE_INT) res.dyn_linkage << 1)
	  | (res.dyn_provideweak ? 1 : 0);
      CONTRACT_DYNAMIC (contract)
	= build_tree_list (get_identifier (res.dyn_name),
			   build_int_cst (uint16_type_node, packed));
    }

  return (contract_evaluation_semantic) sem;
}

/* The result of resolving a contract's caller-side semantic for a specific
   call site: the clamped, compute_semantic-applied SEMANTIC, plus the
   P3595 dynamic-selector descriptor (DYN_NAME == NULL when the resolution
   is not dynamic).  */

struct caller_resolution {
  contract_evaluation_semantic semantic;
  const char *dyn_name;
  unsigned char dyn_linkage;
  bool dyn_provideweak;
};

/* Resolve the caller-side semantic for CONTRACT for a specific call site
   described by CALLER_LOC and CALLER_FNDECL.  This does NOT cache into
   any AST slot -- the result depends on the call site, so it must be
   recomputed per call.  */

static caller_resolution
resolve_caller_semantic (tree contract, tree fndecl,
			 location_t caller_loc, tree caller_fndecl)
{
  contract_query q = make_contract_query (contract, fndecl);
  q.caller_side = true;
  q.in_constant_evaluation = false;
  q.allowed_mask |= (1 << CES_IGNORE);
  q.caller_loc = caller_loc;
  q.caller_fndecl = caller_fndecl;

  auto_vec<const char *> groups_vec;
  fill_query_groups (&q, contract, groups_vec);

  contract_config_result r = contract_config_resolve (&q);
  uint16_t sem = (uint16_t) r.semantic;
  if (sem == CES_INVALID)
    sem = CES_IGNORE;

  /* Apply the label's compute_semantic facet only when caller-side checking
     is actually engaged -- i.e. the resolved caller semantic emits a real
     check (observe/enforce/quick) -- never to the opt-out default
     (ignore/assume).  This preserves the caller-side opt-in model: a call
     site with no matching caller rule resolves to ignore and must stay
     ignore (no wrapper), so a label whose compute_semantic maps
     ignore->observe cannot resurrect a caller-side check that the call site
     never opted into.  Mirrors the callee-side path
     (ensure_evaluation_semantic); uses the caller allowed_mask (which
     includes IGNORE).  */
  if (!contract_semantic_emits_no_check (sem))
    sem = apply_compute_semantic (CONTRACT_LABEL (contract), sem,
				  q.allowed_mask, EXPR_LOCATION (contract));

  caller_resolution out;
  out.semantic = (contract_evaluation_semantic) sem;
  out.dyn_name = r.dyn_name;              /* NULL unless dynamic */
  out.dyn_linkage = r.dyn_linkage;
  out.dyn_provideweak = r.dyn_provideweak;
  return out;
}

/* Constexpr-semantic predicate helpers.  Valid after
   ensure_evaluation_semantic(contract, fndecl, true).  */

bool
contract_constexpr_ignored_p (const_tree contract)
{
  contract_evaluation_semantic s = get_constexpr_evaluation_semantic (contract);
  return s <= CES_IGNORE || contract_semantic_emits_no_check (s);
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

static GTY(()) hash_map<tree, contract_decl> *contract_decl_map;

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

  if (type_dependent_expression_p (condition))
    return condition;

  return condition_conversion (condition);
}

/* Wrap the DECL into VIEW_CONVERT_EXPR representing const qualified version
   of the declaration.  */

tree
view_as_const (tree decl)
{
  if (decl
      && !CP_TYPE_CONST_P (TREE_TYPE (decl)))
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
	 This exemption may change if the design evolves.  */
      if (VAR_P (decl) && DECL_ARTIFICIAL (decl))
	return decl;
      if (REFERENCE_REF_P (decl)
	  && VAR_P (TREE_OPERAND (decl, 0))
	  && DECL_ARTIFICIAL (TREE_OPERAND (decl, 0)))
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

/* Nonzero while substituting the pack of a pack-index-expression that appears
   in a postcondition.  The pack is expanded over every element, but only the
   selected element is odr-used, so per-element checking of the const
   requirement is deferred until the index has selected an element (see
   tsubst_pack_index and check_selected_pack_index_params).  */

bool defer_postcondition_pack_index_check;

/* If declaration DECL is a PARM_DECL and it appears in a postcondition, then
   check that it is not a non-const by-value param. LOCATION is where the
   expression was found and is used for diagnostic purposes.  */

void
check_param_in_postcondition (tree decl, location_t location)
{
  if (processing_postcondition_predicate
      && TREE_CODE (decl) == PARM_DECL
      /* TREE_CODE (decl) == PARM_DECL only holds for non-reference
	 parameters.  */
      && !cp_unevaluated_operand
      /* Return value parameter has DECL_ARTIFICIAL flag set. The flag
	 presence of the flag should be sufficient to distinguish the
	 return value parameter in this context.  */
      && !(DECL_ARTIFICIAL (decl)))
    {
      /* Inside a pack-index-expression (pack...[i]) the pack is expanded over
	 every element, but only the selected element is odr-used.  Defer the
	 check: check_selected_pack_index_params re-runs it on the selected
	 element once the index is known (see tsubst_pack_index).  */
      if (defer_postcondition_pack_index_check)
	return;

      set_parm_used_in_post (decl);

      if (!dependent_type_p (TREE_TYPE (decl))
	  && !CP_TYPE_CONST_P (TREE_TYPE (decl)))
	{
	  auto_diagnostic_group d;
	  error_at (location,
		    "a value parameter used in a postcondition must be const");
	  inform (DECL_SOURCE_LOCATION (decl), "parameter declared here");
	}
    }
}

/* Callback for check_selected_pack_index_params: apply the postcondition
   const check to every PARM_DECL referenced by the selected pack-index
   element.  */

static tree
check_pack_index_param_r (tree *tp, int * /*walk_subtrees*/, void *data)
{
  if (TREE_CODE (*tp) == PARM_DECL)
    check_param_in_postcondition (*tp, *(location_t *) data);
  return NULL_TREE;
}

/* EXPR is the element selected by a pack-index-expression (pack...[i]) that
   appears in a postcondition.  Only that element is odr-used, so -- with
   per-element checking suppressed during the pack expansion -- apply the
   const check to each parameter EXPR references.  LOC is the location of the
   pack-index-expression, used for diagnostics.  */

void
check_selected_pack_index_params (tree expr, location_t loc)
{
  if (!processing_postcondition_predicate
      || expr == NULL_TREE || expr == error_mark_node)
    return;
  cp_walk_tree_without_duplicates (&expr, check_pack_index_param_r, &loc);
}

/* Carry the "odr used in a postcondition" property of the parameter T1 of
   OLDDECL over to the parameter T2 of a redeclaration or instantiation that
   corresponds to it, and check that T2 satisfies the const requirement.  */

static void
check_postcondition_parm_in_redecl (tree olddecl, tree t1, tree t2)
{
  if (!parm_used_in_post_p (t1))
    return;

  set_parm_used_in_post (t2);
  if (!dependent_type_p (TREE_TYPE (t2))
      && !CP_TYPE_CONST_P (TREE_TYPE (t2))
      && !TREE_READONLY (t2))
    {
      auto_diagnostic_group d;
      error_at (DECL_SOURCE_LOCATION (t2),
		"value parameter %qE used in a postcondition must be "
		"const", t2);
      inform (DECL_SOURCE_LOCATION (olddecl), "previous declaration here");
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
     per element while substituting the predicate
     (check_param_in_postcondition, and check_selected_pack_index_params for
     pack indexing).  Only a parameter written BETWEEN two packs is left
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
  for (int i = first_pack < 0 ? len1 : first_pack; i > 0;
       --i, t1 = TREE_CHAIN (t1), t2 = TREE_CHAIN (t2))
    check_postcondition_parm_in_redecl (olddecl, t1, t2);

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
  for (; t1 && t1 != void_list_node && t2 && t2 != void_list_node;
       t1 = TREE_CHAIN (t1), t2 = TREE_CHAIN (t2))
    check_postcondition_parm_in_redecl (olddecl, t1, t2);
}

/* Map from FUNCTION_DECL to a FUNCTION_DECL for either the PRE_FN or POST_FN.
   These are used to parse contract conditions and are called inside the body
   of the guarded function.  */
static GTY(()) hash_map<tree, tree> *decl_pre_fn;
static GTY(()) hash_map<tree, tree> *decl_post_fn;

/* Map from label type -> local violation handler trampoline FUNCTION_DECL.
   Generated at parse time, looked up during gimplification.  */
static GTY(()) hash_map<tree, tree> *local_violation_trampoline_map;

/* Map from label type -> the user's handle_contract_violation FUNCTION_DECL
   that the corresponding trampoline calls.  Recorded so that the rethrow
   analysis (contract_local_handler_always_rethrows_p) examines exactly the
   function the trampoline will call, rather than repeating the member lookup
   and risking a different overload resolution.  */
static GTY(()) hash_map<tree, tree> *local_violation_handler_fn_map;

/* Map from label type -> query trampoline FUNCTION_DECL.
   Generated at parse time, looked up during gimplification.  */
static GTY(()) hash_map<tree, tree> *query_trampoline_map;

/* Given a pre or post function decl (for an outlined check function) return
   the decl for the function for which the outlined checks are being
   performed.  */
static GTY(()) hash_map<tree, tree> *orig_from_outlined;

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

static tree get_postcondition_capture_struct_type (tree);

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
  /* Handle the args list.  */
  tree arg_types = NULL_TREE;
  tree *last = &arg_types;
  for (tree arg_type = TYPE_ARG_TYPES (TREE_TYPE (fn));
      arg_type && arg_type != void_list_node;
      arg_type = TREE_CHAIN (arg_type))
    {
      if (DECL_IOBJ_MEMBER_FUNCTION_P (fndecl)
	  && TYPE_ARG_TYPES (TREE_TYPE (fn)) == arg_type)
      continue;
      *last = build_tree_list (TREE_PURPOSE (arg_type), TREE_VALUE (arg_type));
      last = &TREE_CHAIN (*last);
    }

  /* Copy the function parameters, if present.  Disable warnings for them.  */
  DECL_ARGUMENTS (fn) = NULL_TREE;
  if (DECL_ARGUMENTS (fndecl))
    {
      tree *last_a = &DECL_ARGUMENTS (fn);
      for (tree p = DECL_ARGUMENTS (fndecl); p; p = TREE_CHAIN (p))
	{
	  *last_a = copy_decl (p);
	  suppress_warning (*last_a);
	  DECL_CONTEXT (*last_a) = fn;
	  last_a = &TREE_CHAIN (*last_a);
	}
    }

  tree orig_fn_value_type = TREE_TYPE (TREE_TYPE (fn));
  if (!pre && !VOID_TYPE_P (orig_fn_value_type))
    {
      /* For post contracts that deal with a non-void function, append a
	 parameter to pass the return value.  */
      tree name = get_identifier ("__r");
      tree parm = build_lang_decl (PARM_DECL, name, orig_fn_value_type);
      DECL_CONTEXT (parm) = fn;
      DECL_ARTIFICIAL (parm) = true;
      suppress_warning (parm);
      DECL_ARGUMENTS (fn) = chainon (DECL_ARGUMENTS (fn), parm);
      *last = build_tree_list (NULL_TREE, orig_fn_value_type);
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
	  if (TREE_CODE (contract) != POSTCONDITION_STMT)
	    continue;
	  if (contract_semantic_emits_no_check
		(ensure_evaluation_semantic (contract, fndecl, false)))
	    continue;
	  tree captures = POSTCONDITION_CAPTURES (contract);
	  if (!captures)
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
     Also build a noexcept function (D4298) when every contract this
     outlined function actually checks -- preconditions for the .pre
     function, postconditions for the .post function -- has a statically
     nonthrowing semantic, even if the original function itself may
     throw.  */
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

static bool has_postcondition_captures_p (tree);

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

static GTY(()) hash_map<tree, tree> *decl_wrapper_fn = nullptr;

/* Map from the function decl of a wrapper to the function that it wraps.  */

static GTY(()) hash_map<tree, tree> *decl_for_wrapper = nullptr;

/* Map from a wrapper FUNCTION_DECL to its caller-semantic tuple (a TREE_LIST
   of INTEGER_CSTs, or NULL_TREE for the virtual sentinel).  Read by the
   definition pass to set each copied contract's evaluation semantic.  */

static GTY(()) hash_map<tree, tree> *decl_wrapper_tuple = nullptr;

/* Return true if two caller-semantic tuples are element-wise equal.  Each
   entry's TREE_VALUE is the resolved semantic (INTEGER_CST); its TREE_PURPOSE
   is the P3595 dynamic descriptor (NULL_TREE when the entry is not dynamic,
   else a TREE_LIST whose TREE_PURPOSE is the selector name IDENTIFIER and
   whose TREE_VALUE is the packed linkage/provideweak INTEGER_CST).  Two call
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
	      || tree_to_uhwi (TREE_VALUE (da)) != tree_to_uhwi (TREE_VALUE (db))))
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
   linkage/provideweak INTEGER_CST -- the same layout the callee-side
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

static tree
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

  /* Copy selected attributes from the original function.  */
  TREE_USED (wrapdecl) = TREE_USED (fndecl);

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

  if (!handle_contracts_p (fndecl))
    return;

  /* Check that the postcondition result name, if any, does not shadow a
     function parameter.  */
  if (tree specs = get_fn_contract_specifiers (fndecl))
    for (tree ca : tree_vec_range (specs))
      if (POSTCONDITION_P (ca))
	if (tree id = POSTCONDITION_IDENTIFIER (ca))
	  {
	    if (id == error_mark_node)
	      {
		CONTRACT_CONDITION (ca) = error_mark_node;
		continue;
	      }
	    tree r_name = tree_strip_any_location_wrapper (id);
	    if (TREE_CODE (id) == PARM_DECL)
	      r_name = DECL_NAME (id);
	    gcc_checking_assert (r_name
				 && TREE_CODE (r_name) == IDENTIFIER_NODE);
	    tree seen = lookup_name (r_name);
	    if (seen
		&& TREE_CODE (seen) == PARM_DECL
		&& DECL_CONTEXT (seen) == fndecl)
	      {
		auto_diagnostic_group d;
		location_t id_l = location_wrapper_p (id)
				  ? EXPR_LOCATION (id)
				  : DECL_SOURCE_LOCATION (id);
		location_t co_l = EXPR_LOCATION (ca);
		if (id_l != UNKNOWN_LOCATION)
		  co_l = make_location (id_l, co_l, co_l);
		error_at (co_l, "contract postcondition result name shadows a"
			  " function parameter");
		inform (DECL_SOURCE_LOCATION (seen),
			"parameter declared here");
		POSTCONDITION_IDENTIFIER (ca) = error_mark_node;
		CONTRACT_CONDITION (ca) = error_mark_node;
	      }
	  }

  if (!contract_any_active_p (fndecl))
    return;

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

static void append_capture_struct_args (tree, vec<tree, va_gc> *);

static void
add_pre_condition_fn_call (tree fndecl)
{
  gcc_checking_assert (DECL_PRE_FN (fndecl)
		       && DECL_PRE_FN (fndecl) != error_mark_node);

  releasing_vec args = build_arg_list (fndecl);
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
  if (get_postcondition_result_parameter (fndecl))
    vec_safe_push (args, DECL_RESULT (fndecl));
  append_capture_struct_args (fndecl, args);
  tree call = build_thunk_like_call (DECL_POST_FN (fndecl),
				     args->length (), args->address ());
  finish_expr_stmt (call);
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

      copy_body_data id;
      hash_map<tree, tree> decl_map;

      memset (&id, 0, sizeof (id));

      id.src_fn = fndecl;
      id.dst_fn = fndecl;
      id.src_cfun = DECL_STRUCT_FUNCTION (fndecl);
      id.decl_map = &decl_map;

      id.copy_decl = retain_decl;

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
static void emit_pending_weak_selectors ();

/* Forward declarations for new ABI data block infrastructure.  */
static tree build_contract_data_block_ctor (tree, tree *);
static tree build_contract_data_block_constant (tree, tree, tree);
static tree declare_cxa_entry_point (contract_assertion_kind,
				     contract_evaluation_semantic,
				     int, bool);

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
   because the saved initializer trees must survive to wrapper-emission time.  */
static GTY(()) hash_map<tree, tree> *postcondition_capture_inits;

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
      if (TREE_CODE (contract) != POSTCONDITION_STMT)
	continue;
      if (contract_semantic_emits_no_check
	    (ensure_evaluation_semantic (contract, fndecl, false)))
	continue;
      tree captures = POSTCONDITION_CAPTURES (contract);
      if (captures)
	postconds_with_caps.safe_push (contract);
    }

  /* Destroy in reverse lexical order of postconditions, and within
     each postcondition in reverse order of captures.  Gated by the
     initialized flag -- only destroy if captures were fully constructed.  */
  for (int i = postconds_with_caps.length () - 1; i >= 0; i--)
    {
      tree contract = postconds_with_caps[i];
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
	  tree dtor = build_cleanup (var);
	  if (dtor && dtor != error_mark_node)
	    finish_expr_stmt (dtor);
	}

      if (if_stmt)
	{
	  finish_then_clause (if_stmt);
	  finish_if_stmt (if_stmt);
	}
    }
}

/* Add a call or a direct evaluation of the pre checks.  */

/* Return true if FNDECL has any active postcondition with captures.  */

static bool
has_postcondition_captures_p (tree fndecl)
{
  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return false;

  for (tree contract : tree_vec_range (contracts))
    {
      if (TREE_CODE (contract) == POSTCONDITION_STMT
	  && POSTCONDITION_CAPTURES (contract)
	  && !contract_semantic_emits_no_check
		(ensure_evaluation_semantic (contract, fndecl, false)))
	return true;
    }
  return false;
}

/* Map from postcondition contract -> capture-state RECORD_TYPE.
   Cached because the same type must be shared between __pre_fn/__post_fn
   signatures and the call site.  */
static GTY(()) hash_map<tree, tree> *postcondition_capture_struct_types;

/* Build (or return cached) capture-state struct type for a postcondition
   with captures.  The struct has a bool __initialized field followed by
   union-wrapped fields for each capture (unions prevent implicit
   construction/destruction).  */

static tree
get_postcondition_capture_struct_type (tree contract)
{
  gcc_assert (POSTCONDITION_P (contract));
  gcc_assert (POSTCONDITION_CAPTURES (contract));

  tree *cached = hash_map_safe_get (postcondition_capture_struct_types, contract);
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

  TYPE_FIELDS (struct_type) = fields;
  TYPE_ARTIFICIAL (struct_type) = 1;
  layout_type (struct_type);

  hash_map_maybe_create<hm_ggc> (postcondition_capture_struct_types);
  postcondition_capture_struct_types->put (contract, struct_type);
  return struct_type;
}

/* Map from postcondition contract -> struct local VAR_DECL at the call site.
   Used by outlined mode to share struct locals between add_pre_condition_fn_call
   and add_post_condition_fn_call.  Cleared per function.  */

static hash_map<tree, tree> *outlined_capture_struct_locals;

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
      if (TREE_CODE (contract) != POSTCONDITION_STMT)
	continue;
      if (contract_semantic_emits_no_check
	    (ensure_evaluation_semantic (contract, fndecl, false)))
	continue;
      tree captures = POSTCONDITION_CAPTURES (contract);
      if (!captures)
	continue;

      tree struct_type = get_postcondition_capture_struct_type (contract);

      char buf[32];
      snprintf (buf, sizeof buf, "__cap_struct_%u", idx++);
      tree var = build_decl (UNKNOWN_LOCATION, VAR_DECL,
			     get_identifier (buf), struct_type);
      DECL_ARTIFICIAL (var) = 1;
      DECL_CONTEXT (var) = fndecl;
      layout_decl (var, 0);
      pushdecl (var);
      add_decl_expr (var);

      if (!outlined_capture_struct_locals)
	outlined_capture_struct_locals = new hash_map<tree, tree>;
      outlined_capture_struct_locals->put (contract, var);
    }
}

/* Append capture-state struct arguments to ARGS for an outlined
   pre or post function call.  */

static void
append_capture_struct_args (tree fndecl, vec<tree, va_gc> *args)
{
  if (!outlined_capture_struct_locals)
    return;

  tree contracts = get_fn_contract_specifiers (fndecl);
  if (!contracts)
    return;

  for (tree contract : tree_vec_range (contracts))
    {
      if (TREE_CODE (contract) != POSTCONDITION_STMT)
	continue;
      if (contract_semantic_emits_no_check
	    (ensure_evaluation_semantic (contract, fndecl, false)))
	continue;
      if (!POSTCONDITION_CAPTURES (contract))
	continue;

      tree *struct_var_p = outlined_capture_struct_locals->get (contract);
      gcc_assert (struct_var_p);
      /* build_thunk_like_call skips conversions; we must explicitly take
	 the address for the reference parameter.  */
      vec_safe_push (args, build_address (*struct_var_p));
    }
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
      if (TREE_CODE (contract) != POSTCONDITION_STMT)
	continue;
      if (contract_semantic_emits_no_check
	    (ensure_evaluation_semantic (contract, fndecl, false)))
	continue;
      if (POSTCONDITION_CAPTURES (contract))
	postconds_with_caps.safe_push (contract);
    }

  for (int i = postconds_with_caps.length () - 1; i >= 0; i--)
    {
      tree contract = postconds_with_caps[i];
      tree *struct_var_p = outlined_capture_struct_locals->get (contract);
      if (!struct_var_p)
	continue;

      tree struct_var = *struct_var_p;
      tree struct_type = TREE_TYPE (struct_var);
      tree init_field = TYPE_FIELDS (struct_type);

      tree init_ref = build3 (COMPONENT_REF, TREE_TYPE (init_field),
			      struct_var, init_field, NULL_TREE);

      tree if_stmt = begin_if_stmt ();
      finish_if_stmt_cond (init_ref, if_stmt);

      auto_vec<tree, 4> cap_fields;
      for (tree f = DECL_CHAIN (init_field); f; f = DECL_CHAIN (f))
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

	  tree dtor = build_cleanup (member_ref);
	  if (dtor && dtor != error_mark_node)
	    finish_expr_stmt (dtor);
	}

      finish_then_clause (if_stmt);
      finish_if_stmt (if_stmt);
    }
}

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
   This walks ALL contracts and emits:
   - For preconditions: the check (inline)
   - For postconditions with captures: the capture initialization
   - For postconditions without captures: nothing (handled at postcond site)
   This replaces separate apply_preconditions + capture init
   calls when doing inline emission with captures present.  */

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

  for (tree contract : tree_vec_range (contracts))
    {
      if (TREE_CODE (contract) == PRECONDITION_STMT)
	{
	  /* Emit this precondition (from the copy vector).  */
	  if (pre_ix < pre_len)
	    emit_contract_statement (TREE_VEC_ELT (pre_copies, pre_ix++));
	}
      else if (TREE_CODE (contract) == POSTCONDITION_STMT)
	{
	  /* Emit capture initialization for this postcondition.  */
	  if (contract_semantic_emits_no_check
		(ensure_evaluation_semantic (contract, fndecl, false)))
	    continue;
	  tree captures = POSTCONDITION_CAPTURES (contract);
	  if (!captures)
	    continue;

	  /* Create the initialized flag.  */
	  static unsigned cap_init_counter_interleaved;
	  char buf[32];
	  snprintf (buf, sizeof buf, "__cap_init_%u",
		    cap_init_counter_interleaved++);
	  tree flag = build_decl (UNKNOWN_LOCATION, VAR_DECL,
				  get_identifier (buf), boolean_type_node);
	  DECL_ARTIFICIAL (flag) = 1;
	  DECL_CONTEXT (flag) = fndecl;
	  layout_decl (flag, 0);
	  pushdecl (flag);
	  add_decl_expr (flag);
	  finish_expr_stmt (cp_build_init_expr (flag, boolean_false_node));

	  if (!postcondition_capture_flags)
	    postcondition_capture_flags = new hash_map<tree, tree>;
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

	  /* Try block: initialize captures.  */
	  tree try_block = begin_try_block ();
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
	      if (!postcondition_capture_inits)
		postcondition_capture_inits
		  = hash_map<tree, tree>::create_ggc ();
	      postcondition_capture_inits->put (var, init);
	      finish_expr_stmt (cp_build_init_expr (var, init));
	      DECL_INITIAL (var) = NULL_TREE;
	    }
	  finish_expr_stmt (cp_build_init_expr (flag, boolean_true_node));
	  finish_try_block (try_block);

	  /* Catch handler.  */
	  tree handler = begin_handler ();
	  finish_handler_parms (NULL_TREE, handler);
	  contract_evaluation_semantic sem
	    = ensure_evaluation_semantic (contract, fndecl, false);
	  if (sem == CES_QUICK)
	    finish_expr_stmt
	      (build_quick_enforce_reaction (EXPR_LOCATION (contract)));
	  else
	    {
	      tree block_type;
	      tree ctor = build_contract_data_block_ctor (contract, &block_type);
	      tree data_var = build_contract_data_block_constant (ctor, block_type,
								 contract);
	      tree data_addr = build_address (data_var);
	      tree ep = declare_cxa_entry_point (CAK_POST_CAPTURE, sem,
						CDM_EVAL_EXCEPTION, false);
	      finish_expr_stmt (build_call_n (ep, 1, data_addr));
	    }
	  finish_handler (handler);
	  finish_handler_sequence (try_block);
	}
    }
}

/* Add a call or a direct evaluation of the post checks.
   For postconditions with captures, gate the predicate check on the
   initialized flag (set by the interleaved emission path).  */

static void
apply_postconditions (tree fndecl)
{
  if (flag_contract_checks_outlined && DECL_POST_FN (fndecl))
    {
      add_post_condition_fn_call (fndecl);
      return;
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
	  if (TREE_CODE (orig) == POSTCONDITION_STMT)
	    {
	      orig_contract = orig;
	      break;
	    }
	}

      /* If this postcondition has captures, gate on the initialized flag.  */
      tree *flag_p = NULL;
      if (orig_contract && POSTCONDITION_CAPTURES (orig_contract)
	  && postcondition_capture_flags)
	flag_p = postcondition_capture_flags->get (orig_contract);

      if (flag_p)
	{
	  /* Use the ORIGINAL contract (not the copy) for postconditions with
	     captures, as copy_contracts can remap capture VAR_DECL refs.  */
	  tree if_stmt = begin_if_stmt ();
	  finish_if_stmt_cond (*flag_p, if_stmt);
	  emit_contract_statement (orig_contract);
	  finish_then_clause (if_stmt);
	  finish_if_stmt (if_stmt);
	}
      else
	emit_contract_statement (contract);
    }
}

/* Wrap STMTS -- the postcondition checks of FNDECL -- in a cleanup that
   destroys the returned object if evaluating them exits via an exception,
   which a violation handler that throws will do.

   By the time the checks run the returned object has been initialized:
   [stmt.return]/5 sequences postcondition evaluation after the copy-
   initialization of the result and after the destruction of local variables.
   Unwinding past it without running its destructor leaks an object the
   program can no longer reach.

   No sentinel guard, unlike maybe_splice_retval_cleanup: this region is
   reached only on the normal-completion path of the body, where the returned
   object necessarily exists.  That is also what keeps the two from
   overlapping -- the body's cleanup covers the body and stops there, this one
   covers only the checks -- so the object is destroyed exactly once however
   the function unwinds.

   NOTE this is deliberately more than the standard currently requires.
   [except.ctor]/2 destroys the returned object only for an exception thrown
   "during the destruction of temporaries or local variables for a return
   statement", and does not mention contract assertions; [basic.contract.eval]
   says a throwing handler behaves "as if the function body exits via that
   same exception", which describes a state where the result object was never
   initialized -- not the state we are actually in.  So nothing obliges us to
   run the destructor here.  Leaking is not a defensible answer; a core issue
   is owed, and this should not be "corrected" back to a leak on the strength
   of the wording alone.  */

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
  if (!handle_contracts_p (fndecl))
    /* We did nothing and the original function body statement list will be
       popped by our caller.  */
    return;

  bool do_pre = has_active_preconditions (fndecl);
  bool do_post = has_active_postconditions (fndecl);
  if (!do_pre && !do_post)
    return;

  /* If the function is noexcept, the user's written body will be wrapped in a
     MUST_NOT_THROW expression.  In that case we leave the MUST_NOT_THROW in
     place and do our replacement inside it.  */
  tree fnbody;
  if (TYPE_NOEXCEPT_P (TREE_TYPE (fndecl)))
    {
      tree m_n_t_expr = expr_first (DECL_SAVED_TREE (fndecl));
      gcc_checking_assert (TREE_CODE (m_n_t_expr) == MUST_NOT_THROW_EXPR);
      fnbody = TREE_OPERAND (m_n_t_expr, 0);
      TREE_OPERAND (m_n_t_expr, 0) = push_stmt_list ();
    }
  else
    {
      fnbody = DECL_SAVED_TREE (fndecl);
      DECL_SAVED_TREE (fndecl) = push_stmt_list ();
    }

  /* If we have a lambda with captures, ensure that those captures are in-
     scope for pre and post conditions.  */
  if (LAMBDA_FUNCTION_P (fndecl)
      && TREE_CODE (fnbody) == BIND_EXPR)
    {
      tree extract = BIND_EXPR_BODY (fnbody);
      BIND_EXPR_BODY (fnbody) = NULL_TREE;
      add_stmt (fnbody);
      BIND_EXPR_BODY (fnbody) = push_stmt_list ();
      fnbody = extract;
    }

  /* Now add the pre and post conditions to the existing function body.
     This copies the approach used for function try blocks.  */

  /* We are called from finish_function with the sk_function_parms level
     current, so do_poplevel will see that same level again when it finishes
     the artificial block below -- exactly the test maybe_splice_retval_cleanup
     uses to recognise the function body.  The body's own scope has already
     declared current_retval_sentinel and spliced in its cleanup; doing either
     a second time declares the same VAR_DECL twice (which the gimplifier
     rejects) and would destroy the return value twice on throw.  A contract
     check runs either before the return object exists or after the body's
     cleanup has dealt with it, so this block wants neither.  */
  auto retval_sentinel_ovr = make_temp_override (current_retval_sentinel,
						 NULL_TREE);

  tree compound_stmt = begin_compound_stmt (0);
  current_binding_level->artificial = true;

  /* Do not add locations for the synthesised code.  */
  location_t loc = UNKNOWN_LOCATION;

  /* For other cases, we call a function to process the check.  */

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
    emit_preconditions_and_capture_inits_interleaved (fndecl);
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
      post_stmts = pop_stmt_list (post_stmts);
      TREE_OPERAND (eh_else, 0) = push_stmt_list ();
      add_stmt (wrap_postconditions_in_retval_cleanup (fndecl, post_stmts));
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
      post_stmts = pop_stmt_list (post_stmts);
      add_stmt (wrap_postconditions_in_retval_cleanup (fndecl, post_stmts));
      if (outlined_caps)
	emit_outlined_capture_struct_dtors (fndecl);
      else
	emit_postcondition_capture_dtors (fndecl);
    }
  TREE_OPERAND (try_fin, 1) = pop_stmt_list (TREE_OPERAND (try_fin, 1));
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
	  if (dp && TREE_CODE (contract) == POSTCONDITION_STMT)
	    {
	      gcc_assert (!duplicate_p);
	      if (tree result = POSTCONDITION_IDENTIFIER (contract))
		{
		  gcc_assert (DECL_P (result));
		  insert_decl_map (&id, result, dp);
		  do_remap = true;
		}
	    }
	  break;
	}
      gcc_assert (sp && dp);

      if (sp == dp)
	continue;

      insert_decl_map (&id, sp, dp);
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

  walk_tree (&CONTRACT_CONDITION (contract), copy_tree_body_r, &id, NULL);
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
	  bool virtual_wrapper = (DECL_IOBJ_MEMBER_FUNCTION_P (source)
				  && DECL_VIRTUAL_P (source));
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
		     IDENTIFIER, TREE_VALUE = INTEGER_CST(linkage<<1|weak).  */
		  CONTRACT_DYNAMIC (stmt)
		    = build_tree_list (TREE_PURPOSE (desc), TREE_VALUE (desc));

		  /* The dynamic transform clamps against the caller allowed
		     set, which includes IGNORE.  Fold IGNORE into the mask so
		     the emission (make_contract_query) sees the caller-side
		     set rather than the plain label mask.  */
		  tree m = CONTRACT_ALLOWED_MASK (stmt);
		  uint16_t mask = m ? (uint16_t) tree_to_uhwi (m)
				    : (uint16_t) CES_ALL_ALLOWED_WITH_EXTENSIONS;
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
		 refers to the wrapper's parameters.  Mirrors remap_contract.  */
	      {
		int s_art = num_artificial_parms_for (source);
		int d_art = num_artificial_parms_for (dest);
		for (tree sp = DECL_ARGUMENTS (source), dp = DECL_ARGUMENTS (dest);
		     sp && dp; sp = DECL_CHAIN (sp), dp = DECL_CHAIN (dp))
		  {
		    if (sp != dp)
		      insert_decl_map (&cap_id, sp, dp);
		    if (s_art > 0)
		      while (--s_art, s_art > 0)
			sp = DECL_CHAIN (sp);
		    if (d_art > 0)
		      while (--d_art, d_art > 0)
			dp = DECL_CHAIN (dp);
		  }
	      }

	      tree new_caps = NULL_TREE;
	      tree *last_cap = &new_caps;
	      for (tree cap = caps; cap; cap = TREE_CHAIN (cap))
		{
		  tree orig_var = TREE_VALUE (cap);
		  tree new_var = copy_node (orig_var);
		  cxx_dup_lang_specific_decl (new_var);
		  DECL_CONTEXT (new_var) = dest;
		  /* Recover the initializer if SOURCE's inline emission already
		     cleared it destructively (see postcondition_capture_inits).
		     Structurally copy + remap it (parameters, *this) so the
		     wrapper gets its own initializer and the shared saved tree
		     is not mutated -- otherwise the wrapper's capture is left
		     uninitialized.  */
		  if (!DECL_INITIAL (new_var) && postcondition_capture_inits)
		    if (tree *saved = postcondition_capture_inits->get (orig_var))
		      {
			tree init = *saved;
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
  if (new_contracts && !old_contracts)
    {
      auto_diagnostic_group d;
      /* If a re-declaration has contracts, they must be the same as those
       that appear on the first declaration seen (they cannot be added).  */
      location_t cont_end = get_contract_end_loc (new_contracts);
      cont_end = make_location (new_loc, new_loc, cont_end);
      error_at (cont_end, "declaration adds contracts to %q#D", olddecl);
      inform (DECL_SOURCE_LOCATION (olddecl), "first declared here");
      return;
    }

  if (old_contracts && !new_contracts)
    /* We allow re-declarations to omit contracts declared on the initial decl.
       In fact, this is required if the conditions contain lambdas.  Check if
       all the parameters are correctly const qualified. */
    check_postconditions_in_redecl (olddecl, newdecl);
  else if (old_contracts && new_contracts
	   && !contract_any_deferred_p (old_contracts)
	   && contract_any_deferred_p (new_contracts)
	   && DECL_UNIQUE_FRIEND_P (newdecl))
    {
      /* Put the deferred contracts on the olddecl so we parse it when
	 we can.  */
      set_fn_contract_specifiers (olddecl, old_contracts);
    }
  else if (contract_any_deferred_p (old_contracts)
	   || contract_any_deferred_p (new_contracts))
    {
      /* Known limitation: when either side still has DEFERRED_PARSE contracts at
	 this merge point -- which happens for a friend declaration, whose
	 contracts are late-parsed at the end of the class, while the same
	 function declared outside the class definition is not deferred --
	 redeclaration contract matching is skipped here and is never re-run once
	 the contracts are late-parsed.  A contract *mismatch* between two such
	 declarations (e.g. two friend declarations of the same function with
	 different predicates) is therefore silently accepted rather than
	 diagnosed, unlike every non-deferred redeclaration path.  Diagnosing it
	 would require queuing the deferred contracts and comparing them after
	 late-parse.  See g++.dg/contracts/cpp26/contract-friend-deferred-mismatch.C
	 (xfail).  */
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
    more than we need.  */
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
   contract list (DECL_ORIGIN order), matching copy_and_remap_contracts.  */

static tree
compute_caller_semantic_tuple (tree fndecl, location_t caller_loc,
			       tree caller_fndecl)
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
      /* When the caller-side resolution is dynamic, carry the descriptor in
	 the tuple element's TREE_PURPOSE so it can key the wrapper and be
	 baked into the wrapper's copied contract.  The layout matches the
	 callee-side CONTRACT_DYNAMIC cache: TREE_PURPOSE = selector name
	 IDENTIFIER, TREE_VALUE = INTEGER_CST packing (linkage << 1
	 | provideweak).  TREE_PURPOSE == NULL_TREE means not dynamic.  */
      tree desc = NULL_TREE;
      if (r.dyn_name)
	{
	  unsigned HOST_WIDE_INT packed
	    = ((unsigned HOST_WIDE_INT) r.dyn_linkage << 1)
	      | (r.dyn_provideweak ? 1 : 0);
	  desc = build_tree_list (get_identifier (r.dyn_name),
				  build_int_cst (uint16_type_node, packed));
	}
      tree node = build_tree_list (desc,
				   build_int_cst (uint16_type_node, r.semantic));
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
  if (!is_virtual)
    {
      tuple = compute_caller_semantic_tuple (fndecl, input_location,
					     current_function_decl);
      for (tree t = tuple; t; t = TREE_CHAIN (t))
	/* A dynamic descriptor (TREE_PURPOSE non-null) forces the wrapper to
	   be emitted even when the compile-time default is ignore/assume: the
	   selector may return a checking semantic at run time.  This mirrors
	   the callee-side contract_active_p, which forces active whenever
	   CONTRACT_DYNAMIC is present.  Gating on an actual descriptor (not on
	   a label merely having a compute_semantic facet) preserves the
	   caller-side opt-in invariant.  */
	if (TREE_PURPOSE (t)
	    || !contract_semantic_emits_no_check (tree_to_uhwi (TREE_VALUE (t))))
	  {
	    any_active = true;
	    break;
	  }
    }

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

  bool is_virtual = (DECL_IOBJ_MEMBER_FUNCTION_P (fndecl)
		     && DECL_VIRTUAL_P (fndecl));

  /* For virtual functions, always include postconditions -- the wrapper uses
     callee-side semantics (P3097).  For non-virtual, check whether any
     postcondition is active caller-side according to this wrapper's stored
     caller-semantic tuple.  Positions are counted over the full contract
     list, matching compute_caller_semantic_tuple / copy_and_remap_contracts.  */
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

  /* D4298: unlike build_contract_condition_function's outlined .pre/.post
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

  tree call = build_thunk_like_call (fn, args->length (), args->address ());

  finish_return_stmt (call);

  finish_compound_stmt (compound_stmt);
  finish_function_body (body);
  expand_or_defer_fn (finish_function (/*inline_p=*/false));
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
  return define_one_contract_wrapper_func (fndecl, wrapdecl);
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

/* If any wrapper functions have been declared, emit their definition.
   This might be called multiple times, as we instantiate functions. When
   the processing here adds more wrappers, then flag to the caller that
   possible additional instantiations should be considered.
   Once instantiations are complete, this will be called with done == true.  */

/* Return the total number of (tuple, wrapper) pairs recorded across all
   callees in decl_wrapper_fn.  A single callee may have several wrappers.  */

static size_t
count_wrapper_pairs (void)
{
  if (!decl_wrapper_fn)
    return 0;
  size_t n = 0;
  for (hash_map<tree, tree>::iterator it = decl_wrapper_fn->begin ();
       it != decl_wrapper_fn->end (); ++it)
    for (tree p = (*it).second; p; p = TREE_CHAIN (p))
      n++;
  return n;
}

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
			  tree current_val, location_t /*loc*/)
{
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
  if (!fn || fn == error_mark_node)
    return current_val;

  tree arg = current_val ? current_val
			 : build_zero_cst (const_string_type_node);
  vec<tree, va_gc> *args = NULL;
  vec_safe_push (args, arg);
  tree call = build_new_method_call (label, fn, &args,
				     NULL_TREE, LOOKUP_NORMAL,
				     NULL, tf_none);
  if (!call || call == error_mark_node)
    return current_val;

  tree result = cxx_constant_value (call, NULL_TREE, tf_none);
  tree str = extract_string_from_const_char_ptr (result);
  if (str)
    {
      if (!current_val
	  || (current_val && TREE_CODE (current_val) == STRING_CST))
	return build_string (TREE_STRING_LENGTH (str),
			     TREE_STRING_POINTER (str));
      return build_string_literal (TREE_STRING_LENGTH (str),
				   TREE_STRING_POINTER (str));
    }
  return current_val;
}

/* Make a string of the contract condition, if it is available.  */

static tree
build_comment (cp_expr condition)
{
  /* Try to get the actual source text for the condition; if that fails pretty
     print the resulting tree.  */
  char *str = get_source_text_between (global_dc->get_file_cache (),
				       condition.get_start (),
				       condition.get_finish ());
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
  if (message)
    message = tree_strip_any_location_wrapper (message);
  if (message && TREE_CODE (message) != STRING_CST)
    {
      cexpr_str cstr (message);
      if (!cstr.type_check (loc))
	message = NULL_TREE;
      else if (TREE_CODE (condition) != DEFERRED_PARSE)
	{
	  const char *msg;
	  int len;
	  if (cstr.extract (loc, msg, len))
	    {
	      char *buf = XNEWVEC (char, len + 1);
	      memcpy (buf, msg, len);
	      buf[len] = '\0';
	      message = build_string (len + 1, buf);
	      XDELETEVEC (buf);
	    }
	  else
	    message = NULL_TREE;
	}
    }
  CONTRACT_MESSAGE (contract) = message;

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
      if (!aco || aco == error_mark_node)
	{
	  error_at (loc, "type %qT does not satisfy "
		    "%<assertion_control_object%> "
		    "(missing nested type %<assertion_control_object%>)",
		    label_type);
	  CONTRACT_LABEL (contract) = NULL_TREE;
	  return;
	}
    }

  /* Check for local_violation_label facet and generate trampoline
     if needed (P3400).  Keyed by label type so the same trampoline
     is reused across all contracts with the same label type.  */
  if (CONTRACT_LABEL (contract) && CLASS_TYPE_P (label_type))
    {
      if (!local_violation_trampoline_map)
	local_violation_trampoline_map = hash_map<tree, tree>::create_ggc ();

      if (!local_violation_trampoline_map->get (label_type))
	{
	  tree hcv_id = get_identifier ("handle_contract_violation");
	  tree hcv_fn = lookup_member (label_type, hcv_id,
				       /*protect=*/0, /*want_type=*/false,
				       tf_none);
	  if (hcv_fn && hcv_fn != error_mark_node)
	    {
	      /* Test viability using the library's contract_violation
		 type.  */
	      tree cv_base_type = lookup_std_contracts_type (
		get_identifier ("contract_violation"));
	      tree cv_const_type = cp_build_qualified_type (
		cv_base_type, TYPE_QUAL_CONST);
	      tree cv_ptr_type = build_pointer_type (cv_const_type);
	      tree dummy_ref = cp_build_indirect_ref (
		input_location, build_zero_cst (cv_ptr_type),
		RO_UNARY_STAR, tf_none);
	      if (dummy_ref && dummy_ref != error_mark_node)
		{
		  vec<tree, va_gc> *test_args = NULL;
		  vec_safe_push (test_args, dummy_ref);
		  tree test_call = build_new_method_call (
		    label, hcv_fn, &test_args, NULL_TREE,
		    LOOKUP_NORMAL, NULL, tf_none);
		  if (!test_call || test_call == error_mark_node)
		    hcv_fn = NULL_TREE;
		}
	      else
		hcv_fn = NULL_TREE;
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
		      if (!local_violation_handler_fn_map)
			local_violation_handler_fn_map
			  = hash_map<tree, tree>::create_ggc ();
		      local_violation_handler_fn_map->put (label_type,
							  resolved_fn);
		    }
		}
	    }
	}

    }

  /* Check for queryable_label facet and generate query trampoline
     if needed (P3400).  Keyed by label type so the same trampoline
     is reused across all contracts with the same label type.  */
  if (CONTRACT_LABEL (contract) && CLASS_TYPE_P (label_type))
    {
      if (!query_trampoline_map)
	query_trampoline_map = hash_map<tree, tree>::create_ggc ();

      if (!query_trampoline_map->get (label_type))
	{
	  tree query_id = get_identifier ("query");
	  tree query_fn = lookup_member (label_type, query_id,
					/*protect=*/0, /*want_type=*/false,
					tf_none);
	  if (query_fn && query_fn != error_mark_node)
	    {
	      /* Test viability: query(const void*, size_t) -> void*.  */
	      tree const_void_ptr = build_pointer_type (
		cp_build_qualified_type (void_type_node, TYPE_QUAL_CONST));
	      vec<tree, va_gc> *test_args = NULL;
	      vec_safe_push (test_args,
			     build_zero_cst (const_void_ptr));
	      vec_safe_push (test_args,
			     build_zero_cst (size_type_node));
	      tree test_call = build_new_method_call (
		label, query_fn, &test_args, NULL_TREE,
		LOOKUP_NORMAL, NULL, tf_none);
	      if (test_call && test_call != error_mark_node
		  && POINTER_TYPE_P (TREE_TYPE (test_call))
		  && VOID_TYPE_P (TREE_TYPE (TREE_TYPE (test_call))))
		{
		  tree trampoline = build_query_trampoline (label_type);
		  if (trampoline)
		    query_trampoline_map->put (label_type, trampoline);
		}
	    }
	}
    }

  /* If the label needs its address taken (local handler or query
     trampoline) but is not already a VAR_DECL, materialize it as
     a static variable.  */
  if (CONTRACT_LABEL (contract) && CLASS_TYPE_P (label_type))
    {
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
	     cxx_constant_value and passing the result in was wrong twice
	     over: on failure it returned error_mark_node and the whole
	     materialization was skipped in silence, leaving the label a
	     prvalue so the facet was never wired up at all; and on success
	     it produced a syntactic COMPOUND_LITERAL_P CONSTRUCTOR, which
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

  if (CONTRACT_LABEL (contract) && CLASS_TYPE_P (label_type))
    {
      tree as_member = lookup_member (label_type,
				     get_identifier ("allowed_semantics"),
				     /*protect=*/0, /*want_type=*/false,
				     tf_none);
      if (as_member && as_member != error_mark_node)
	{
	  /* label.allowed_semantics is a member OBJECT; call
	     .contains(sem) on it to test each semantic.  */
	  auto is_allowed = [&](uint16_t sem_val) -> bool
	  {
	    tree as_val = finish_class_member_access_expr
	      (label, get_identifier ("allowed_semantics"), false,
	       tf_none);
	    if (!as_val || as_val == error_mark_node)
	      return true;
	    tree contains_fn = lookup_member
	      (TREE_TYPE (as_val), get_identifier ("contains"),
	       /*protect=*/0, /*want_type=*/false, tf_none);
	    if (!contains_fn || contains_fn == error_mark_node)
	      return true;
	    tree fn_decl = (TREE_CODE (contains_fn) == BASELINK
			    ? BASELINK_FUNCTIONS (contains_fn)
			    : contains_fn);
	    if (TREE_CODE (fn_decl) == OVERLOAD)
	      fn_decl = OVL_FIRST (fn_decl);
	    tree parm = FUNCTION_FIRST_USER_PARMTYPE (fn_decl);
	    tree sem_type = parm ? TREE_VALUE (parm) : uint16_type_node;
	    vec<tree, va_gc> *args = NULL;
	    vec_safe_push (args, build_int_cst (sem_type, sem_val));
	    tree call = build_new_method_call
	      (as_val, contains_fn, &args, NULL_TREE,
	       LOOKUP_NORMAL, NULL, tf_none);
	    if (!call || call == error_mark_node)
	      return true;
	    tree r = cxx_constant_value (call, NULL_TREE, tf_none);
	    if (r && TREE_CODE (r) == INTEGER_CST)
	      return tree_to_uhwi (r) != 0;
	    return true;
	  };

	  uint16_t restricted = 0;
	  for (uint16_t sem = CES_IGNORE; sem <= CES_NOEXCEPT_ENFORCE; sem++)
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
	       tree requires_clause)
{
  if (condition == error_mark_node)
    return error_mark_node;

  tree_code code;
  contract_assertion_kind kind = CAK_INVALID;
  /* Both the standard spelling "contract_assert" and the extension spelling
     "__contract_assert" (see c-common.cc) tokenize to RID_CONTASSERT.  */
  if (id_equal (contract_spec, "contract_assert")
      || id_equal (contract_spec, "__contract_assert"))
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
      /* POSTCONDITION_STMT: 14 operands; result (identifier) at op 12.  */
      contract = build_nt (code,
			   NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE,
			   NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE,
			   NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE,
			   result, NULL_TREE);
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
      CONTRACT_CONDITION (contract) = condition;
      return contract;
    }

  /* Generate the comment from the original condition.  */
  CONTRACT_COMMENT (contract) = build_comment (condition);

  /* Apply compute_comment facet (P3400) if present.  */
  CONTRACT_COMMENT (contract)
    = apply_label_string_facet (label, "compute_comment",
				CONTRACT_COMMENT (contract), loc);

  /* The condition is converted to bool.  */
  condition = finish_contract_condition (condition);

  if (condition == error_mark_node)
    return error_mark_node;

  CONTRACT_CONDITION (contract) = condition;

  return contract;
}

/* Update condition of a late-parsed contract and postcondition variable,
   if any.  */

void
update_late_contract (tree contract, tree result, cp_expr condition)
{
  if (TREE_CODE (contract) == POSTCONDITION_STMT)
    POSTCONDITION_IDENTIFIER (contract) = result;

  /* Generate the comment from the original condition.  */
  CONTRACT_COMMENT (contract) = build_comment (condition);

  /* Apply compute_comment facet (P3400) if present.  */
  tree label = CONTRACT_LABEL (contract);
  CONTRACT_COMMENT (contract)
    = apply_label_string_facet (label, "compute_comment",
				CONTRACT_COMMENT (contract),
				EXPR_LOCATION (contract));

  /* The condition is converted to bool.  */
  condition = finish_contract_condition (condition);
  CONTRACT_CONDITION (contract) = condition;
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
      if (TREE_CODE (contract) == POSTCONDITION_STMT
	  && POSTCONDITION_CAPTURES (contract)
	  && !contract_semantic_emits_no_check
		(ensure_evaluation_semantic (contract, fndecl, false)))
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

  bool do_remap = false;
  int src_num_artificial = num_artificial_parms_for (src);
  int dst_num_artificial = num_artificial_parms_for (dst);

  for (tree sp = DECL_ARGUMENTS (src), dp = DECL_ARGUMENTS (dst);
       sp && dp;
       sp = DECL_CHAIN (sp), dp = DECL_CHAIN (dp))
    {
      if (sp != dp)
	{
	  insert_decl_map (id, sp, dp);
	  do_remap = true;
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

  return do_remap;
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
      else if (TREE_CODE (contract) == POSTCONDITION_STMT
	       && !contract_semantic_emits_no_check
		    (ensure_evaluation_semantic (contract, fndecl, false))
	       && POSTCONDITION_CAPTURES (contract))
	{
	  tree struct_param = struct_params[struct_idx++];
	  tree struct_ref = convert_from_reference (struct_param);
	  tree struct_type = TREE_TYPE (struct_ref);
	  tree init_field = TYPE_FIELDS (struct_type);

	  /* Set __initialized = false.  */
	  tree init_ref = build3 (COMPONENT_REF, TREE_TYPE (init_field),
				  struct_ref, init_field, NULL_TREE);
	  finish_expr_stmt (cp_build_init_expr (init_ref, boolean_false_node));

	  /* Try block: initialize each capture field.  */
	  tree try_block = begin_try_block ();

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
	      finish_expr_stmt (cp_build_init_expr (member_ref, remapped_init));
	    }

	  /* All inits succeeded: set __initialized = true.  */
	  finish_expr_stmt (cp_build_init_expr (init_ref, boolean_true_node));
	  finish_try_block (try_block);

	  /* Catch (...): call violation handler.  */
	  tree handler = begin_handler ();
	  finish_handler_parms (NULL_TREE, handler);

	  contract_evaluation_semantic sem
	    = ensure_evaluation_semantic (contract, fndecl, false);
	  if (sem == CES_QUICK)
	    finish_expr_stmt
	      (build_quick_enforce_reaction (EXPR_LOCATION (contract)));
	  else
	    {
	      tree block_type;
	      tree ctor = build_contract_data_block_ctor (contract, &block_type);
	      tree data_var = build_contract_data_block_constant (ctor, block_type,
								 contract);
	      tree data_addr = build_address (data_var);
	      tree ep = declare_cxa_entry_point (CAK_POST_CAPTURE, sem,
						CDM_EVAL_EXCEPTION, false);
	      finish_expr_stmt (build_call_n (ep, 1, data_addr));
	    }

	  finish_handler (handler);
	  finish_handler_sequence (try_block);
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
	= (captures
	   && !contract_semantic_emits_no_check
		(ensure_evaluation_semantic (contract, fndecl, false)));

      tree c = copy_node (contract);
      if (CONTRACT_CONDITION (c) != error_mark_node)
	remap_contract (fndecl, post_fn, c, /*duplicate_p=*/false);

      if (!has_captures)
	{
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

      /* Remap capture references in the condition.  */
      walk_tree (&CONTRACT_CONDITION (c), remap_capture_vars_r, &cap_map,
		 NULL);

      /* Gate predicate evaluation on __initialized.  */
      tree init_ref = build3 (COMPONENT_REF, TREE_TYPE (init_field),
			      struct_ref, init_field, NULL_TREE);
      tree if_stmt = begin_if_stmt ();
      finish_if_stmt_cond (init_ref, if_stmt);
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
     caller-side).  The config system governs whether any contract is active.  */
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

static GTY(()) tree tu_quick_enforce_wrapper = NULL_TREE;

/* Declare a noipa wrapper around the quick_enforce trap.  */

static tree
declare_quick_enforce_wrapper ()
{
  if (tu_quick_enforce_wrapper)
    return tu_quick_enforce_wrapper;

  iloc_sentinel ils (input_location);
  input_location = BUILTINS_LOCATION;

  tree fn_type = build_function_type_list (void_type_node, NULL_TREE);
  fn_type = build_exception_variant (fn_type, noexcept_true_spec);
  tree fn_name = get_identifier ("__tu_quick_enforce_wrapper");

  tu_quick_enforce_wrapper
    = build_lang_decl_loc (input_location, FUNCTION_DECL, fn_name, fn_type);
  DECL_CONTEXT (tu_quick_enforce_wrapper) = FROB_CONTEXT(global_namespace);
  DECL_ARTIFICIAL (tu_quick_enforce_wrapper) = true;
  DECL_INITIAL (tu_quick_enforce_wrapper) = error_mark_node;
  /* Let the start function code fill in the result decl.  */
  DECL_RESULT (tu_quick_enforce_wrapper) = NULL_TREE;

  /* Make this function internal.  */
  TREE_PUBLIC (tu_quick_enforce_wrapper) = false;
  DECL_EXTERNAL (tu_quick_enforce_wrapper) = false;
  DECL_WEAK (tu_quick_enforce_wrapper) = false;

  DECL_ATTRIBUTES (tu_quick_enforce_wrapper)
    = tree_cons (get_identifier ("noipa"), NULL, NULL_TREE);
  cplus_decl_attributes (&tu_quick_enforce_wrapper,
			 DECL_ATTRIBUTES (tu_quick_enforce_wrapper), 0);
  return tu_quick_enforce_wrapper;
}

/* Define the noipa wrapper: it just traps.  */

static void
build_quick_enforce_wrapper ()
{
  /* We should not be trying to build this if we never used it.  */
  gcc_checking_assert (tu_quick_enforce_wrapper);

  start_preparsed_function (tu_quick_enforce_wrapper,
			    DECL_ATTRIBUTES(tu_quick_enforce_wrapper),
			    SF_DEFAULT | SF_PRE_PARSED);
  tree body = begin_function_body ();
  tree compound_stmt = begin_compound_stmt (BCS_FN_BODY);
  finish_expr_stmt (build_call_expr_loc (BUILTINS_LOCATION,
					 builtin_decl_explicit (BUILT_IN_TRAP),
					 0));
  finish_return_stmt (NULL_TREE);
  finish_compound_stmt (compound_stmt);
  finish_function_body (body);
  tu_quick_enforce_wrapper = finish_function (false);
  expand_or_defer_fn (tu_quick_enforce_wrapper);
}

/* P2900 quick_enforce: terminate the program via __builtin_trap () -- no
   handler is invoked.  With -fcontracts-conservative-ipa (the default) the trap
   is emitted inside a noipa wrapper so inter-procedural analysis cannot use a
   contract check to optimize callers, which would be incorrect when the same
   assertion may be evaluated differently (e.g. ignore) in another TU
   (BZ121936).  Otherwise the trap is emitted inline.  */

static tree
build_quick_enforce_reaction (location_t loc)
{
  if (flag_contracts_conservative_ipa)
    return build_call_a (declare_quick_enforce_wrapper (), 0, nullptr);
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
    error_at (loc, "%<::handle_contract_violation%> shall not be"
	      " declared %<inline%>");

  if (module_attach_p ())
    error_at (loc, "%<::handle_contract_violation%> shall be attached"
	      " to the global module");

  if (DECL_LANGUAGE (fndecl) != lang_cplusplus)
    error_at (loc, "%<::handle_contract_violation%> shall have"
	      " C++ language linkage");

  tree fntype = TREE_TYPE (fndecl);
  if (!same_type_p (TREE_TYPE (fntype), void_type_node))
    error_at (loc, "%<::handle_contract_violation%> shall return %<void%>");

  tree parms = TYPE_ARG_TYPES (fntype);
  if (!parms || parms == void_list_node)
    {
      error_at (loc, "%<::handle_contract_violation%> shall have a single"
		" parameter of type %<const std::contracts::"
		"contract_violation&%>");
      return;
    }

  tree parmtype = TREE_VALUE (parms);
  tree remaining = TREE_CHAIN (parms);
  if (remaining != void_list_node)
    {
      error_at (loc, "%<::handle_contract_violation%> shall have a single"
		" parameter of type %<const std::contracts::"
		"contract_violation&%>");
      return;
    }

  if (!TYPE_REF_P (parmtype)
      || TYPE_REF_IS_RVALUE (parmtype))
    {
      error_at (loc, "parameter of %<::handle_contract_violation%> shall be"
		" an lvalue reference to %<const std::contracts::"
		"contract_violation%>");
      return;
    }

  tree reftype = TREE_TYPE (parmtype);
  if (!CP_TYPE_CONST_P (reftype))
    {
      error_at (loc, "parameter of %<::handle_contract_violation%> shall be"
		" a reference to %<const std::contracts::"
		"contract_violation%>");
      return;
    }

  tree viol_type = lookup_std_contracts_type (
    get_identifier ("contract_violation"));
  if (!same_type_ignoring_top_level_qualifiers_p (reftype, viol_type))
    error_at (loc, "parameter of %<::handle_contract_violation%> shall be"
	      " of type %<const std::contracts::contract_violation&%>");
}

/* Emit a C-linkage alias __handle_contract_violation for the user's
   ::handle_contract_violation, if defined in this TU.  */

static void
maybe_emit_hcv_alias ()
{
  if (!TARGET_SUPPORTS_ALIASES)
    return;

  tree fnname = get_identifier ("handle_contract_violation");
  tree l = lookup_qualified_name (global_namespace, fnname,
				  LOOK_want::HIDDEN_FRIEND);
  tree fndecl = NULL_TREE;
  for (tree f: lkp_range (l))
    if (TREE_CODE (f) == FUNCTION_DECL && DECL_INITIAL (f) != NULL_TREE)
      {
	fndecl = f;
	break;
      }

  if (!fndecl)
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

/* Emit any TU-level contract infrastructure (descriptor tables, etc.).
   Called at end of translation unit from cp_write_global_declarations.  */

void
maybe_emit_violation_handler_wrappers ()
{
  if (tu_quick_enforce_wrapper && flag_contracts_conservative_ipa)
    build_quick_enforce_wrapper ();
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

/* Build a trampoline function for local violation handlers (P3400).
   The generated function has signature:
     int __trampoline(const void* label_ptr, const void* violation_ptr)
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

  /* Generate a unique internal name for the trampoline.  */
  static int trampoline_counter = 0;
  char name[64];
  snprintf (name, sizeof (name), "__contract_local_handler_%d",
	    trampoline_counter++);

  /* Build function type: int(const void*, const void*)
     This matches __cxa_local_handler_fn_t in the ABI.  */
  tree const_void_ptr = build_pointer_type (
    cp_build_qualified_type (void_type_node, TYPE_QUAL_CONST));

  tree fn_type = build_function_type_list (integer_type_node,
					   const_void_ptr,
					   const_void_ptr,
					   NULL_TREE);

  /* Declare the function.  */
  location_t loc = BUILTINS_LOCATION;
  tree fn_decl = build_lang_decl_loc (loc, FUNCTION_DECL,
				      get_identifier (name), fn_type);
  DECL_CONTEXT (fn_decl) = FROB_CONTEXT (global_namespace);
  DECL_ARTIFICIAL (fn_decl) = true;
  DECL_INITIAL (fn_decl) = error_mark_node;
  DECL_RESULT (fn_decl) = NULL_TREE;
  tree p1 = cp_build_parm_decl (fn_decl, NULL_TREE, const_void_ptr);
  TREE_USED (p1) = true;
  DECL_READ_P (p1) = true;
  tree p2 = cp_build_parm_decl (fn_decl, NULL_TREE, const_void_ptr);
  TREE_USED (p2) = true;
  DECL_READ_P (p2) = true;
  DECL_CHAIN (p1) = p2;
  DECL_ARGUMENTS (fn_decl) = p1;
  TREE_PUBLIC (fn_decl) = false;
  DECL_EXTERNAL (fn_decl) = false;
  DECL_WEAK (fn_decl) = false;

  /* Save the enclosing parse state for the duration; see
     trampoline_scope.  */
  trampoline_scope sentry;

  /* Define the function body.  */
  start_preparsed_function (fn_decl, NULL_TREE, SF_DEFAULT | SF_PRE_PARSED);
  tree body = begin_function_body ();
  tree compound_stmt = begin_compound_stmt (BCS_FN_BODY);

  tree parm_label_ptr = DECL_ARGUMENTS (fn_decl);
  tree parm_violation_ptr = DECL_CHAIN (parm_label_ptr);

  /* Cast: LabelType& lbl = *(LabelType*) label_ptr;  */
  tree label_ptr_type = build_pointer_type (label_type);
  tree cast_label = build1 (NOP_EXPR, label_ptr_type, parm_label_ptr);
  tree label_ref = cp_build_indirect_ref (loc, cast_label,
					  RO_UNARY_STAR, tf_warning_or_error);

  /* Cast: const contract_violation& v = *(const contract_violation*) ptr;  */
  tree cv_type = lookup_std_contracts_type (get_identifier ("contract_violation"));
  tree cv_const = cp_build_qualified_type (cv_type, TYPE_QUAL_CONST);
  tree cv_ptr_type = build_pointer_type (cv_const);
  tree cast_viol = build1 (NOP_EXPR, cv_ptr_type, parm_violation_ptr);
  tree violation_ref = cp_build_indirect_ref (loc, cast_viol,
					      RO_UNARY_STAR, tf_warning_or_error);

  /* Call: lbl.handle_contract_violation(violation);  */
  vec<tree, va_gc> *args = NULL;
  vec_safe_push (args, violation_ref);
  tree call = build_new_method_call (label_ref, hcv_fn, &args,
				     NULL_TREE, LOOKUP_NORMAL,
				     NULL, tf_none);

  if (!call || call == error_mark_node)
    {
      finish_compound_stmt (compound_stmt);
      finish_function_body (body);
      finish_function (false);
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
      tree int_result = build1 (NOP_EXPR, integer_type_node, call);
      finish_return_stmt (int_result);
    }

  finish_compound_stmt (compound_stmt);
  finish_function_body (body);
  fn_decl = finish_function (false);
  expand_or_defer_fn (fn_decl);

  return fn_decl;
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
  static int query_trampoline_counter = 0;
  char name[64];
  snprintf (name, sizeof (name), "__contract_query_%d",
	    query_trampoline_counter++);

  /* Build function type: void*(const void*, const void*, size_t).  */
  tree const_void_ptr = build_pointer_type (
    cp_build_qualified_type (void_type_node, TYPE_QUAL_CONST));
  tree void_ptr = build_pointer_type (void_type_node);

  tree fn_type = build_function_type_list (void_ptr,
					   const_void_ptr,
					   const_void_ptr,
					   size_type_node,
					   NULL_TREE);

  /* Declare the function.  */
  location_t loc = BUILTINS_LOCATION;
  tree fn_decl = build_lang_decl_loc (loc, FUNCTION_DECL,
				      get_identifier (name), fn_type);
  DECL_CONTEXT (fn_decl) = FROB_CONTEXT (global_namespace);
  DECL_ARTIFICIAL (fn_decl) = true;
  DECL_INITIAL (fn_decl) = error_mark_node;
  DECL_RESULT (fn_decl) = NULL_TREE;

  tree p1 = cp_build_parm_decl (fn_decl, NULL_TREE, const_void_ptr);
  TREE_USED (p1) = true;
  DECL_READ_P (p1) = true;
  tree p2 = cp_build_parm_decl (fn_decl, NULL_TREE, const_void_ptr);
  TREE_USED (p2) = true;
  DECL_READ_P (p2) = true;
  tree p3 = cp_build_parm_decl (fn_decl, NULL_TREE, size_type_node);
  TREE_USED (p3) = true;
  DECL_READ_P (p3) = true;
  DECL_CHAIN (p1) = p2;
  DECL_CHAIN (p2) = p3;
  DECL_ARGUMENTS (fn_decl) = p1;
  TREE_PUBLIC (fn_decl) = false;
  DECL_EXTERNAL (fn_decl) = false;
  DECL_WEAK (fn_decl) = false;

  /* Save the enclosing parse state for the duration; see
     trampoline_scope.  */
  trampoline_scope sentry;

  /* Define the function body.  */
  start_preparsed_function (fn_decl, NULL_TREE, SF_DEFAULT | SF_PRE_PARSED);
  tree body = begin_function_body ();
  tree compound_stmt = begin_compound_stmt (BCS_FN_BODY);

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
      finish_compound_stmt (compound_stmt);
      finish_function_body (body);
      finish_function (false);
      return NULL_TREE;
    }

  /* Return the result (void*).  */
  finish_return_stmt (call);

  finish_compound_stmt (compound_stmt);
  finish_function_body (body);
  fn_decl = finish_function (false);
  expand_or_defer_fn (fn_decl);

  return fn_decl;
}

/* Build a named TU-local constant of TYPE.  */

static tree
contracts_tu_local_named_var (location_t loc, const char *name, tree type)
{
  tree var_ = build_decl (loc, VAR_DECL, NULL, type);
  DECL_NAME (var_) = generate_internal_label (name);
  TREE_PUBLIC (var_) = false;
  DECL_EXTERNAL (var_) = false;
  TREE_STATIC (var_) = true;
  DECL_ARTIFICIAL (var_) = true;
  TREE_CONSTANT (var_) = true;
  layout_decl (var_, 0);
  return var_;
}

/* Helper to replace references to dummy this parameters with references to
   the first argument of the FUNCTION_DECL DATA.  */

static tree
remap_dummy_this_1 (tree *tp, int *, void *data)
{
  if (!is_this_parameter (*tp))
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
  data.to = DECL_RESULT (fndecl);
  walk_tree (&CONTRACT_CONDITION (contract), remap_retval_1, &data, NULL);
}


/* Genericize a CONTRACT tree, but do not attach it to the current context,
   the caller is responsible for that.
   This is called during genericization.  */

/* === New ABI data block infrastructure ===

   The compiler emits __cxa_contract_data_block structs in .rodata and calls
   __cxa_contract_violation_* entry points defined in libstdc++exp.
   See bits/contracts_abi.h for the ABI specification.  */

/* Data block RECORD_TYPEs.  The "basic" variant has source_location + comment
   + message.  The "label" variant adds local_handler + label_ptr.  The
   "query" variant adds query_function + label_ptr.  The "full" variant adds
   local_handler + query_function + label_ptr.  */
static GTY(()) tree contract_data_block_basic_type;
static GTY(()) tree contract_data_block_label_type;
static GTY(()) tree contract_data_block_query_type;
static GTY(()) tree contract_data_block_full_type;

/* Descriptor table RECORD_TYPEs and static const instances (one per TU).  */
static GTY(()) tree contract_desc_basic_type;
static GTY(()) tree contract_desc_basic_var;
static GTY(()) tree contract_desc_label_type;
static GTY(()) tree contract_desc_label_var;
static GTY(()) tree contract_desc_query_type;
static GTY(()) tree contract_desc_query_var;
static GTY(()) tree contract_desc_full_type;
static GTY(()) tree contract_desc_full_var;

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
  if (contract_data_block_basic_type)
    return;

  /* Basic data block:
     { void* descriptor, void* next,
       const char* file, const char* function, unsigned line, unsigned column,
       const char* comment, const char* message }  */
  {
    const tree types[] = {
      ptr_type_node,		    /* descriptor */
      ptr_type_node,		    /* next */
      const_string_type_node,	    /* file */
      const_string_type_node,	    /* function */
      unsigned_type_node,	    /* line */
      unsigned_type_node,	    /* column */
      const_string_type_node,	    /* comment */
      const_string_type_node,	    /* message */
    };
    const char *names[] = {
      "_descriptor", "_next",
      "_file", "_function", "_line", "_column",
      "_comment", "_message",
    };
    contract_data_block_basic_type
      = build_record_type_from_arrays ("__contract_data_block_basic",
				       types, names, 8);
  }

  /* Label data block: basic + local_handler + label_ptr.  */
  {
    const tree types[] = {
      ptr_type_node,		    /* descriptor */
      ptr_type_node,		    /* next */
      const_string_type_node,	    /* file */
      const_string_type_node,	    /* function */
      unsigned_type_node,	    /* line */
      unsigned_type_node,	    /* column */
      const_string_type_node,	    /* comment */
      const_string_type_node,	    /* message */
      ptr_type_node,		    /* local_handler */
      ptr_type_node,		    /* label_ptr */
    };
    const char *names[] = {
      "_descriptor", "_next",
      "_file", "_function", "_line", "_column",
      "_comment", "_message",
      "_local_handler", "_label_ptr",
    };
    contract_data_block_label_type
      = build_record_type_from_arrays ("__contract_data_block_label",
				       types, names, 10);
  }

  /* Query data block: basic + query_function + label_ptr.  */
  {
    const tree types[] = {
      ptr_type_node,		    /* descriptor */
      ptr_type_node,		    /* next */
      const_string_type_node,	    /* file */
      const_string_type_node,	    /* function */
      unsigned_type_node,	    /* line */
      unsigned_type_node,	    /* column */
      const_string_type_node,	    /* comment */
      const_string_type_node,	    /* message */
      ptr_type_node,		    /* query_function */
      ptr_type_node,		    /* label_ptr */
    };
    const char *names[] = {
      "_descriptor", "_next",
      "_file", "_function", "_line", "_column",
      "_comment", "_message",
      "_query_function", "_label_ptr",
    };
    contract_data_block_query_type
      = build_record_type_from_arrays ("__contract_data_block_query",
				       types, names, 10);
  }

  /* Full data block: basic + local_handler + query_function + label_ptr.  */
  {
    const tree types[] = {
      ptr_type_node,		    /* descriptor */
      ptr_type_node,		    /* next */
      const_string_type_node,	    /* file */
      const_string_type_node,	    /* function */
      unsigned_type_node,	    /* line */
      unsigned_type_node,	    /* column */
      const_string_type_node,	    /* comment */
      const_string_type_node,	    /* message */
      ptr_type_node,		    /* local_handler */
      ptr_type_node,		    /* query_function */
      ptr_type_node,		    /* label_ptr */
    };
    const char *names[] = {
      "_descriptor", "_next",
      "_file", "_function", "_line", "_column",
      "_comment", "_message",
      "_local_handler", "_query_function", "_label_ptr",
    };
    contract_data_block_full_type
      = build_record_type_from_arrays ("__contract_data_block_full",
				       types, names, 11);
  }
}

/* Build a descriptor table RECORD_TYPE with N entries.
   Layout: header(u8), num_entries(u8), fid[N](u8 each), pad, off[N](uintptr each).
   Returns the type and sets *out_fields to the field list.  */

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
      names.safe_push (xstrdup (fid_name));
    }

  /* Offset values (uintptr_t).  GCC may add padding before these.  */
  char off_name[32];
  for (unsigned i = 0; i < num_entries; i++)
    {
      snprintf (off_name, sizeof (off_name), "_off_%u", i);
      types.safe_push (size_type_node);
      names.safe_push (xstrdup (off_name));
    }

  return build_record_type_from_arrays (name, types.address (),
					names.address (), num_fields);
}

/* Get the Nth field of a RECORD_TYPE, walking DECL_CHAIN.  */

static tree
get_nth_field (tree record_type, unsigned n)
{
  tree f = TYPE_FIELDS (record_type);
  for (unsigned i = 0; i < n; i++)
    f = next_aggregate_field (DECL_CHAIN (f));
  return f ? next_aggregate_field (f) : f;
}

/* Initialize and emit descriptor table constants for this TU.
   Call once per TU, when contracts are first seen.  */

static void
init_contract_descriptor_tables ()
{
  if (contract_desc_basic_var)
    return;

  init_contract_data_block_types ();

  /* Basic descriptor: 3 entries (source_location, comment, message).  */
  contract_desc_basic_type
    = build_descriptor_table_type ("__contract_desc_basic", 3);

  /* Compute field offsets from the basic data block type.  */
  tree db = contract_data_block_basic_type;
  tree f_file = get_nth_field (db, 2);	   /* _file */
  tree f_comment = get_nth_field (db, 6);  /* _comment */
  tree f_message = get_nth_field (db, 7);  /* _message */

  tree d3 = contract_desc_basic_type;
  tree d3_f = TYPE_FIELDS (d3);
  /* Fields: header, num_entries, fid0, fid1, fid2, off0, off1, off2.  */
  tree d3_fields[8];
  {
    tree f = d3_f;
    for (int i = 0; i < 8; i++)
      {
	d3_fields[i] = next_aggregate_field (f);
	f = DECL_CHAIN (d3_fields[i]);
      }
  }

  /* header: version=1, vendor=GCC => (1 << 4) | 1 = 0x11.  */
  tree ctor = build_constructor_va
    (d3, 8,
     d3_fields[0], build_int_cst (unsigned_char_type_node, 0x11),
     d3_fields[1], build_int_cst (unsigned_char_type_node, 3),
     d3_fields[2], build_int_cst (unsigned_char_type_node, 0x01),
     d3_fields[3], build_int_cst (unsigned_char_type_node, 0x02),
     d3_fields[4], build_int_cst (unsigned_char_type_node, 0x03),
     d3_fields[5], byte_position (f_file),
     d3_fields[6], byte_position (f_comment),
     d3_fields[7], byte_position (f_message));
  TREE_CONSTANT (ctor) = true;
  TREE_READONLY (ctor) = true;

  contract_desc_basic_var
    = contracts_tu_local_named_var (BUILTINS_LOCATION,
				    "Lcontract_desc_basic", d3);
  DECL_INITIAL (contract_desc_basic_var) = ctor;
  varpool_node::finalize_decl (contract_desc_basic_var);
  /* These descriptor tables are shared TU-local statics, finalized lazily when
     the first contract data block in the TU is built.  That first block may
     belong to an inline/COMDAT library function (e.g. under a catch-all
     "kind: implicit" configuration that matches implicit assertions in
     <contracts>' own inline code); if all such early referrers are later
     reclaimed by symtab_remove_unreachable_nodes, the descriptor would be
     removed too -- yet a middle-end check instrumented later in pass_ubsan can
     still reference it, leaving a dangling reference at link time.  Force the
     descriptors to be emitted so a late referrer always resolves.  */
  varpool_node::get (contract_desc_basic_var)->force_output = true;

  /* Label descriptor: 5 entries (source_location, comment, message,
     local_handler, label_ptr).  */
  contract_desc_label_type
    = build_descriptor_table_type ("__contract_desc_label", 5);

  tree db_l = contract_data_block_label_type;
  tree fl_file = get_nth_field (db_l, 2);
  tree fl_comment = get_nth_field (db_l, 6);
  tree fl_message = get_nth_field (db_l, 7);
  tree fl_handler = get_nth_field (db_l, 8);
  tree fl_label = get_nth_field (db_l, 9);

  tree d5 = contract_desc_label_type;
  tree d5_fields[12];
  {
    tree f = TYPE_FIELDS (d5);
    for (int i = 0; i < 12; i++)
      {
	d5_fields[i] = next_aggregate_field (f);
	f = DECL_CHAIN (d5_fields[i]);
      }
  }

  tree ctor5 = build_constructor_va
    (d5, 12,
     d5_fields[0], build_int_cst (unsigned_char_type_node, 0x11),
     d5_fields[1], build_int_cst (unsigned_char_type_node, 5),
     d5_fields[2], build_int_cst (unsigned_char_type_node, 0x01),
     d5_fields[3], build_int_cst (unsigned_char_type_node, 0x02),
     d5_fields[4], build_int_cst (unsigned_char_type_node, 0x03),
     d5_fields[5], build_int_cst (unsigned_char_type_node, 0x04),
     d5_fields[6], build_int_cst (unsigned_char_type_node, 0x06),
     d5_fields[7], byte_position (fl_file),
     d5_fields[8], byte_position (fl_comment),
     d5_fields[9], byte_position (fl_message),
     d5_fields[10], byte_position (fl_handler),
     d5_fields[11], byte_position (fl_label));
  TREE_CONSTANT (ctor5) = true;
  TREE_READONLY (ctor5) = true;

  contract_desc_label_var
    = contracts_tu_local_named_var (BUILTINS_LOCATION,
				    "Lcontract_desc_label", d5);
  DECL_INITIAL (contract_desc_label_var) = ctor5;
  varpool_node::finalize_decl (contract_desc_label_var);
  varpool_node::get (contract_desc_label_var)->force_output = true;

  /* Query descriptor: 5 entries (source_location, comment, message,
     query_function, label_ptr).  */
  contract_desc_query_type
    = build_descriptor_table_type ("__contract_desc_query", 5);

  tree db_q = contract_data_block_query_type;
  tree fq_file = get_nth_field (db_q, 2);
  tree fq_comment = get_nth_field (db_q, 6);
  tree fq_message = get_nth_field (db_q, 7);
  tree fq_query = get_nth_field (db_q, 8);
  tree fq_label = get_nth_field (db_q, 9);

  tree dq = contract_desc_query_type;
  tree dq_fields[12];
  {
    tree f = TYPE_FIELDS (dq);
    for (int i = 0; i < 12; i++)
      {
	dq_fields[i] = next_aggregate_field (f);
	f = DECL_CHAIN (dq_fields[i]);
      }
  }

  tree ctor_q = build_constructor_va
    (dq, 12,
     dq_fields[0], build_int_cst (unsigned_char_type_node, 0x11),
     dq_fields[1], build_int_cst (unsigned_char_type_node, 5),
     dq_fields[2], build_int_cst (unsigned_char_type_node, 0x01),
     dq_fields[3], build_int_cst (unsigned_char_type_node, 0x02),
     dq_fields[4], build_int_cst (unsigned_char_type_node, 0x03),
     dq_fields[5], build_int_cst (unsigned_char_type_node, 0x05),
     dq_fields[6], build_int_cst (unsigned_char_type_node, 0x06),
     dq_fields[7], byte_position (fq_file),
     dq_fields[8], byte_position (fq_comment),
     dq_fields[9], byte_position (fq_message),
     dq_fields[10], byte_position (fq_query),
     dq_fields[11], byte_position (fq_label));
  TREE_CONSTANT (ctor_q) = true;
  TREE_READONLY (ctor_q) = true;

  contract_desc_query_var
    = contracts_tu_local_named_var (BUILTINS_LOCATION,
				    "Lcontract_desc_query", dq);
  DECL_INITIAL (contract_desc_query_var) = ctor_q;
  varpool_node::finalize_decl (contract_desc_query_var);
  varpool_node::get (contract_desc_query_var)->force_output = true;

  /* Full descriptor: 6 entries (source_location, comment, message,
     local_handler, query_function, label_ptr).  */
  contract_desc_full_type
    = build_descriptor_table_type ("__contract_desc_full", 6);

  tree db_f = contract_data_block_full_type;
  tree ff_file = get_nth_field (db_f, 2);
  tree ff_comment = get_nth_field (db_f, 6);
  tree ff_message = get_nth_field (db_f, 7);
  tree ff_handler = get_nth_field (db_f, 8);
  tree ff_query = get_nth_field (db_f, 9);
  tree ff_label = get_nth_field (db_f, 10);

  tree df = contract_desc_full_type;
  tree df_fields[14];
  {
    tree f = TYPE_FIELDS (df);
    for (int i = 0; i < 14; i++)
      {
	df_fields[i] = next_aggregate_field (f);
	f = DECL_CHAIN (df_fields[i]);
      }
  }

  tree ctor_f = build_constructor_va
    (df, 14,
     df_fields[0], build_int_cst (unsigned_char_type_node, 0x11),
     df_fields[1], build_int_cst (unsigned_char_type_node, 6),
     df_fields[2], build_int_cst (unsigned_char_type_node, 0x01),
     df_fields[3], build_int_cst (unsigned_char_type_node, 0x02),
     df_fields[4], build_int_cst (unsigned_char_type_node, 0x03),
     df_fields[5], build_int_cst (unsigned_char_type_node, 0x04),
     df_fields[6], build_int_cst (unsigned_char_type_node, 0x05),
     df_fields[7], build_int_cst (unsigned_char_type_node, 0x06),
     df_fields[8], byte_position (ff_file),
     df_fields[9], byte_position (ff_comment),
     df_fields[10], byte_position (ff_message),
     df_fields[11], byte_position (ff_handler),
     df_fields[12], byte_position (ff_query),
     df_fields[13], byte_position (ff_label));
  TREE_CONSTANT (ctor_f) = true;
  TREE_READONLY (ctor_f) = true;

  contract_desc_full_var
    = contracts_tu_local_named_var (BUILTINS_LOCATION,
				    "Lcontract_desc_full", df);
  DECL_INITIAL (contract_desc_full_var) = ctor_f;
  varpool_node::finalize_decl (contract_desc_full_var);
  varpool_node::get (contract_desc_full_var)->force_output = true;
}

/* Build a data block constructor for CONTRACT.
   Returns a tree for the constructor and sets *out_type to the type used.  */

static tree
build_contract_data_block_ctor (tree contract, tree *out_type)
{
  init_contract_descriptor_tables ();

  location_t loc = EXPR_LOCATION (contract);

  /* Determine if this assertion has a label with facets needing runtime data.  */
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

  /* Select block type and descriptor.  */
  tree block_type;
  tree desc_var;
  unsigned nfields;

  if (has_handler && has_query)
    {
      block_type = contract_data_block_full_type;
      desc_var = contract_desc_full_var;
      nfields = 11;
    }
  else if (has_handler)
    {
      block_type = contract_data_block_label_type;
      desc_var = contract_desc_label_var;
      nfields = 10;
    }
  else if (has_query)
    {
      block_type = contract_data_block_query_type;
      desc_var = contract_desc_query_var;
      nfields = 10;
    }
  else
    {
      block_type = contract_data_block_basic_type;
      desc_var = contract_desc_basic_var;
      nfields = 8;
    }

  /* Get source location components.  */
  tree fndecl = current_function_decl;
  if (DECL_IS_PRE_FN_P (fndecl) || DECL_IS_POST_FN_P (fndecl))
    fndecl = get_orig_for_outlined (fndecl);
  if (DECL_IS_WRAPPER_FN_P (fndecl))
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

  /* Build the descriptor pointer.  */
  tree desc_ptr = build_address (desc_var);

  /* Build the constructor.  */
  tree fields[11];
  {
    tree f = TYPE_FIELDS (block_type);
    for (unsigned i = 0; i < nfields; i++)
      {
	fields[i] = next_aggregate_field (f);
	f = DECL_CHAIN (fields[i]);
      }
  }

  tree ctor;
  if (has_handler && has_query)
    ctor = build_constructor_va
      (block_type, 11,
       fields[0], desc_ptr,
       fields[1], build_zero_cst (ptr_type_node),  /* next = null */
       fields[2], file_str,
       fields[3], func_str,
       fields[4], line_val,
       fields[5], col_val,
       fields[6], comment,
       fields[7], message,
       fields[8], local_handler_val,
       fields[9], query_function_val,
       fields[10], label_ptr_val);
  else if (has_handler)
    ctor = build_constructor_va
      (block_type, 10,
       fields[0], desc_ptr,
       fields[1], build_zero_cst (ptr_type_node),  /* next = null */
       fields[2], file_str,
       fields[3], func_str,
       fields[4], line_val,
       fields[5], col_val,
       fields[6], comment,
       fields[7], message,
       fields[8], local_handler_val,
       fields[9], label_ptr_val);
  else if (has_query)
    ctor = build_constructor_va
      (block_type, 10,
       fields[0], desc_ptr,
       fields[1], build_zero_cst (ptr_type_node),  /* next = null */
       fields[2], file_str,
       fields[3], func_str,
       fields[4], line_val,
       fields[5], col_val,
       fields[6], comment,
       fields[7], message,
       fields[8], query_function_val,
       fields[9], label_ptr_val);
  else
    ctor = build_constructor_va
      (block_type, 8,
       fields[0], desc_ptr,
       fields[1], build_zero_cst (ptr_type_node),  /* next = null */
       fields[2], file_str,
       fields[3], func_str,
       fields[4], line_val,
       fields[5], col_val,
       fields[6], comment,
       fields[7], message);

  TREE_READONLY (ctor) = true;
  TREE_CONSTANT (ctor) = true;

  *out_type = block_type;
  return ctor;
}

/* Create a read-only data block constant.  */

static tree
build_contract_data_block_constant (tree ctor, tree block_type, tree contract)
{
  tree var = contracts_tu_local_named_var (EXPR_LOCATION (contract),
					  "Lcontract_data", block_type);
  DECL_INITIAL (var) = ctor;
  varpool_node::finalize_decl (var);
  return var;
}

/* Return the entry point name for the given combination.  */

static const char *
get_cxa_entry_point_name (contract_assertion_kind kind,
			  contract_evaluation_semantic semantic,
			  int detection_mode,
			  bool is_noexcept)
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
  switch (detection_mode)
    {
    case CDM_PREDICATE_FALSE: mode_str = "pf"; break;
    case CDM_EVAL_EXCEPTION: mode_str = "ex"; break;
    default: gcc_unreachable ();
    }

  char buf[128];
  if (is_noexcept)
    snprintf (buf, sizeof (buf),
	      "__cxa_contract_violation_%s_%s_%s_noexcept",
	      kind_str, sem_str, mode_str);
  else
    snprintf (buf, sizeof (buf),
	      "__cxa_contract_violation_%s_%s_%s",
	      kind_str, sem_str, mode_str);

  return ggc_strdup (buf);
}

/* Cached entry point declarations, keyed by name.  */
static GTY(()) hash_map<nofree_string_hash, tree> *cxa_entry_point_cache;

/* Declare (or return cached) a __cxa_contract_violation_* entry point.  */

static tree
declare_cxa_entry_point (contract_assertion_kind kind,
			 contract_evaluation_semantic semantic,
			 int detection_mode,
			 bool is_noexcept)
{
  const char *name = get_cxa_entry_point_name (kind, semantic,
					       detection_mode, is_noexcept);

  if (!cxa_entry_point_cache)
    cxa_entry_point_cache = hash_map<nofree_string_hash, tree>::create_ggc (16);

  tree *cached = cxa_entry_point_cache->get (name);
  if (cached)
    return *cached;

  bool is_noreturn = (semantic == CES_ENFORCE
		      || semantic == CES_NOEXCEPT_ENFORCE);
  tree fntype = build_function_type_list (void_type_node,
					  ptr_type_node, NULL_TREE);
  if (is_noexcept)
    fntype = build_exception_variant (fntype, NULL_TREE);
  tree fndecl = build_lang_decl (FUNCTION_DECL,
				 get_identifier (name), fntype);
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
   and observe semantics -- quick_enforce calls no handler, and the D4298
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

static inline aval
av_unknown ()
{
  aval a = { AV_UNKNOWN, 0 };
  return a;
}

static inline aval
av_const (HOST_WIDE_INT v)
{
  aval a = { AV_CONST, v };
  return a;
}

static inline aval
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
  RO_RETHROWN,		/* Control left by rethrowing the in-flight exception.  */
  RO_RETURNED,		/* Control returned normally, having done nothing
			   observable.  */
  RO_FAIL		/* Could not be analysed, or something observable
			   happened.  */
};

/* How deep the walk will follow calls before giving up.  A handler that
   delegates more than this far is not a shape worth proving, and the limit
   doubles as the termination guard for mutual recursion.  */

static const int RETHROW_MAX_DEPTH = 8;

/* Strip the conversions and address/indirection operators that stand between
   a use of a declaration and the declaration itself.  */

static tree
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

static bool
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
	 under -g.  It carries no code.  Falling into the default below
	 instead silently switched the whole analysis off in any debug build
	 -- which is most real builds, and every Compiler Explorer session,
	 since CE always passes -g.  */
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

/* Emit the check body for CONTRACT under a single, statically known
   evaluation SEMANTIC.  Returns a BIND_EXPR statement expression, or
   void_node when the semantic emits no check (ignore/assume), or
   NULL_TREE on error.  This is the per-semantic core shared by the plain
   (compile-time-resolved) path and the P3595 dynamic-dispatch path.

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
      /* P3100 "assume" emits no check for now, exactly like "ignore".  */
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
  bool is_noexcept = (semantic == CES_NOEXCEPT_ENFORCE
		      || semantic == CES_NOEXCEPT_OBSERVE);

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
  if (!quick && calls_handler)
    {
      if (shared_data_addr)
	/* Reuse the block built once for all dynamic-dispatch arms.  */
	data_addr = shared_data_addr;
      else
	{
	  /* Build a data block for the violation.  */
	  tree block_type;
	  tree ctor = build_contract_data_block_ctor (contract, &block_type);
	  tree data_var = build_contract_data_block_constant (ctor, block_type,
							      contract);
	  data_addr = build_address (data_var);
	}
    }

  /* Get the entry points we will call.  */
  tree entry_pf = NULL_TREE;
  tree entry_ex = NULL_TREE;
  if (calls_handler)
    {
      entry_pf = declare_cxa_entry_point (kind, semantic,
					  CDM_PREDICATE_FALSE, is_noexcept);
      if (check_might_throw)
	entry_ex = declare_cxa_entry_point (kind, semantic,
					    CDM_EVAL_EXCEPTION, is_noexcept);
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
	{
	  /* Call the _ex variant with the SAME data block.
	     Detection mode (evaluation_exception) is encoded in the entry
	     point name, not the data.  */
	  finish_expr_stmt (build_call_n (entry_ex, 1, data_addr));
	  tree e = cp_build_modify_expr (loc, check_failed, NOP_EXPR,
					 boolean_false_node,
					 tf_warning_or_error);
	  finish_expr_stmt (e);
	}
      finish_handler (handler);
      finish_handler_sequence (check_try);
      cond = check_failed;
      BIND_EXPR_VARS (cc_bind) = nreverse (BIND_EXPR_VARS (cc_bind));
    }

  tree do_check = begin_if_stmt ();
  finish_if_stmt_cond (cond, do_check);
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
   violation as if an "enforce" predicate had evaluated false, then
   terminate.  Used as the default arm of the P3595 dynamic-dispatch
   switch, for a selector value that is unknown or maps to no valid
   semantic (P3595R0).  Returns a BIND_EXPR statement expression, or
   NULL_TREE on error.  SHARED_DATA_ADDR is the address of the violation data
   block already built once for this contract's dynamic dispatch; it is reused
   here (the block is identical to the one every dispatch arm uses).  */

static tree
emit_enforced_violation (tree contract, tree shared_data_addr)
{
  contract_assertion_kind kind = get_contract_assertion_kind (contract);

  tree cc_bind = build3 (BIND_EXPR, void_type_node, NULL, NULL, NULL);
  BIND_EXPR_BODY (cc_bind) = push_stmt_list ();

  /* Reuse the violation data block built once for the dynamic dispatch.  */
  tree data_addr = shared_data_addr;

  /* Call the enforce predicate-false entry point unconditionally.  That
     entry point is noreturn for CES_ENFORCE, so no explicit terminate is
     required after it.  */
  tree entry_pf = declare_cxa_entry_point (kind, CES_ENFORCE,
					   CDM_PREDICATE_FALSE, false);
  finish_expr_stmt (build_call_n (entry_pf, 1, data_addr));

  BIND_EXPR_BODY (cc_bind) = pop_stmt_list (BIND_EXPR_BODY (cc_bind));
  return cc_bind;
}

/* P3100: build the GENERIC code to append at the fall-off point of a
   value-returning function whose implicit {stmt.return.flow.off} assertion
   resolved to SEM.  FNDECL is the function and LOC the site.  Returns a
   statement (or STATEMENT_LIST) to append to the function body, or NULL_TREE
   when the caller should keep the legacy behaviour (SEM == CES_ASSUME) or when
   no code is needed.

   Reaching the end of a value-returning function is always a violation, so the
   emitted code is an unconditional reaction, not a guarded check:

     ignore			 -> return a defined (erroneous) value; no handler
     quick_enforce		 -> call the terminate handler (noreturn)
     enforce / noexcept_enforce	 -> call the noreturn violation entry point
     observe / noexcept_observe	 -> call the (returning) violation entry point,
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
     no indeterminate data is leaked regardless of the return type.  For a scalar
     this is a zero value; for a class/array the whole object storage is cleared
     with memset (every byte, including padding) -- well-defined for a
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

  /* enforce / observe / noexcept_enforce / noexcept_observe: build a violation
     data block and call the corresponding CAK_IMPLICIT entry point.  */
  bool is_noexcept = (sem == CES_NOEXCEPT_ENFORCE
		      || sem == CES_NOEXCEPT_OBSERVE);

  tree contract = make_node (ASSERTION_STMT);
  TREE_TYPE (contract) = void_type_node;
  SET_EXPR_LOCATION (contract, loc);
  CONTRACT_COMMENT (contract)
    = build_string_literal ("control reached the end of a value-returning "
			    "function");

  tree block_type;
  tree ctor = build_contract_data_block_ctor (contract, &block_type);
  tree data_var = build_contract_data_block_constant (ctor, block_type,
						      contract);
  tree data_addr = build_address (data_var);

  tree entry = declare_cxa_entry_point (CAK_IMPLICIT, sem,
					CDM_PREDICATE_FALSE, is_noexcept);
  tree call = build_call_n (entry, 1, data_addr);
  SET_EXPR_LOCATION (call, loc);

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

/* P3100: build the reaction for control flowing off the end of a coroutine whose
   promise type has no usable return_void ({stmt.return.coroutine.flow.off}), for
   the resolved semantic SEM.  This is the coroutine analogue of
   build_implicit_flow_off_check, but there is no return value to substitute: the
   coroutine's return object was created at get_return_object, so the continuing
   semantics simply proceed to the final suspend.  Returns

     assume / ignore			 -> NULL_TREE (fall through to final
					    suspend, as today);
     quick_enforce			 -> call the terminate handler (noreturn);
     enforce / noexcept_enforce		 -> call the noreturn violation entry point;
     observe / noexcept_observe		 -> call the (returning) violation entry
					    point, then continue.

   The reaction is emitted inside the coroutine's try block, so a throwing
   enforce/observe is caught by promise.unhandled_exception ().  Reported through
   the CAK_IMPLICIT entry points (assertion_kind::implicit).  Builds GENERIC.  */

tree
build_implicit_coroutine_flow_off_check (tree fndecl, location_t loc,
					 contract_evaluation_semantic sem)
{
  (void) fndecl;   /* Kept for signature symmetry with the flow-off builder.  */
  if (sem == CES_ASSUME || sem == CES_IGNORE)
    return NULL_TREE;

  if (sem == CES_QUICK)
    return build_quick_enforce_reaction (loc);

  bool is_noexcept = (sem == CES_NOEXCEPT_ENFORCE
		      || sem == CES_NOEXCEPT_OBSERVE);

  tree contract = make_node (ASSERTION_STMT);
  TREE_TYPE (contract) = void_type_node;
  SET_EXPR_LOCATION (contract, loc);
  CONTRACT_COMMENT (contract)
    = build_string_literal ("control flowed off the end of a coroutine");

  tree block_type;
  tree ctor = build_contract_data_block_ctor (contract, &block_type);
  tree data_var = build_contract_data_block_constant (ctor, block_type,
						      contract);
  tree data_addr = build_address (data_var);

  tree entry = declare_cxa_entry_point (CAK_IMPLICIT, sem,
					CDM_PREDICATE_FALSE, is_noexcept);
  tree call = build_call_n (entry, 1, data_addr);
  SET_EXPR_LOCATION (call, loc);
  /* enforce/noexcept_enforce: the entry point is noreturn.  observe/
     noexcept_observe: it returns and control continues to the final suspend.
     Either way the call is the whole reaction -- no defined return.  */
  return call;
}

/* P3100: select the __cxa_pure_virtual terminus for a pure virtual.

   A call that dispatches to a pure virtual function
   ({class.abstract.pure.virtual}) is core-language UB.  The "check" here is the
   vtable slot itself: instead of the legacy __cxa_pure_virtual (which prints a
   message and terminates), point the slot at a semantic-specific terminus that
   reports the violation through the contract-violation handler as an implicit
   (assertion_kind::implicit) assertion.  The slot stays a plain function
   pointer -- only its default value changes.

   The semantic is resolved HERE, where the vtable is emitted, using the class's
   own definition location and namespace, so per-file/line and per-namespace
   P3595 configuration selects it per class.  (The vtable is emitted once per
   program for a class with a key function -- in that function's translation
   unit -- and in every user translation unit otherwise; the configuration
   active where the vtable is built therefore governs, and may differ across
   translation units.  That is an accepted consequence of a shared terminus with
   no per-call site.)

   FN_ORIGINAL is the pure virtual filling the slot; its declared exception
   specification chooses between the throwing terminus and the noexcept
   (terminate-on-throw) terminus, so a throwing handler on a noexcept pure
   virtual still terminates rather than escaping into a caller that assumed the
   call could not throw.

   Returns the terminus FUNCTION_DECL, or NULL_TREE when the caller should keep
   the legacy __cxa_pure_virtual: -fcontracts-p3100 off, or the resolved
   semantic is assume/ignore (a pure-virtual call has no defined value to
   substitute, so ignore is the status quo).  */

tree
build_implicit_pure_virtual_terminus (tree fn_original)
{
  if (!flag_contracts_p3100)
    return NULL_TREE;

  /* Resolve at the pure virtual's own (base) class definition: the config that
     applies where that class is defined governs every vtable that carries the
     slot, deterministically across translation units.  */
  tree class_type = DECL_CONTEXT (fn_original);
  tree class_decl = (class_type && TYPE_P (class_type))
		    ? TYPE_MAIN_DECL (class_type) : NULL_TREE;
  location_t loc = class_decl ? DECL_SOURCE_LOCATION (class_decl)
			      : input_location;
  contract_evaluation_semantic sem
    = resolve_implicit_contract_semantic (class_decl, loc,
					  "ub:class.abstract.pure.virtual");

  /* assume/ignore keep the status quo: a pure-virtual call has no defined value
     to substitute, so there is nothing for ignore to do beyond the legacy
     terminus.  */
  if (sem == CES_ASSUME || sem == CES_IGNORE)
    return NULL_TREE;

  /* A throwing handler must not escape a noexcept pure virtual, so promote a
     throwing enforce/observe to its terminate-on-throw (noexcept) terminus.  */
  if (TYPE_NOTHROW_P (TREE_TYPE (fn_original)))
    {
      if (sem == CES_ENFORCE)
	sem = CES_NOEXCEPT_ENFORCE;
      else if (sem == CES_OBSERVE)
	sem = CES_NOEXCEPT_OBSERVE;
    }

  const char *name;
  int ecf = ECF_NORETURN | ECF_COLD;
  switch (sem)
    {
    case CES_QUICK:
      name = "__cxa_pure_virtual_quick";
      ecf |= ECF_NOTHROW;
      break;
    case CES_ENFORCE:
      name = "__cxa_pure_virtual_enforce";
      break;
    case CES_OBSERVE:
      name = "__cxa_pure_virtual_observe";
      break;
    case CES_NOEXCEPT_ENFORCE:
      name = "__cxa_pure_virtual_noexcept_enforce";
      ecf |= ECF_NOTHROW;
      break;
    case CES_NOEXCEPT_OBSERVE:
      name = "__cxa_pure_virtual_noexcept_observe";
      ecf |= ECF_NOTHROW;
      break;
    default:
      /* assume/ignore were handled above; every other semantic is one of the
	 five termini.  */
      gcc_unreachable ();
    }

  tree id = get_identifier (name);
  tree fn = get_global_binding (id);
  if (!fn)
    fn = push_library_fn (id,
			  build_function_type_list (void_type_node, NULL_TREE),
			  NULL_TREE, ecf);
  return fn;
}

/* P3100: build the runtime check for a configurable [[assume (COND)]] whose site
   resolves to a *checking* semantic SEM (never CES_ASSUME/CES_IGNORE).  COND is
   the assumed predicate, which the caller has determined is side-effect-free and
   evaluable (see build_assume_call), so it is safe to evaluate here even though
   [[assume]] normally never evaluates its operand.  Returns

     if (!COND) <reaction>;

   where <reaction> reports/terminates for SEM through the CAK_IMPLICIT
   contract-violation entry points, so a handler observes
   assertion_kind::implicit (the assumed condition being false is
   {dcl.attr.assume.false.pure} -- this function only ever runs for the
   checkable/pure subset; see build_assume_call).  The site is inside the
   function body, so a throwing
   enforce/observe handler can unwind -- the full front-end semantic set is
   available, exactly like the other front-end implicit checks.  The caller
   appends the optimizer "assume" hint for the enforcing family (COND then
   provably holds).  Runs during parsing/instantiation, so it builds GENERIC.  */

tree
cp_build_assume_check (location_t loc, tree cond,
		       contract_evaluation_semantic sem)
{
  gcc_checking_assert (sem != CES_ASSUME && sem != CES_IGNORE);

  tree reaction;
  if (sem == CES_QUICK)
    reaction = build_quick_enforce_reaction (loc);
  else
    {
      bool is_noexcept = (sem == CES_NOEXCEPT_ENFORCE
			  || sem == CES_NOEXCEPT_OBSERVE);
      tree contract = make_node (ASSERTION_STMT);
      TREE_TYPE (contract) = void_type_node;
      SET_EXPR_LOCATION (contract, loc);
      CONTRACT_COMMENT (contract)
	= build_string_literal ("assumed condition is false");

      tree block_type;
      tree ctor = build_contract_data_block_ctor (contract, &block_type);
      tree data_var = build_contract_data_block_constant (ctor, block_type,
							  contract);
      tree data_addr = build_address (data_var);

      tree entry = declare_cxa_entry_point (CAK_IMPLICIT, sem,
					    CDM_PREDICATE_FALSE, is_noexcept);
      reaction = build_call_n (entry, 1, data_addr);
      SET_EXPR_LOCATION (reaction, loc);
      /* enforce / noexcept_enforce: the entry point is noreturn.
	 observe / noexcept_observe: it returns and execution continues (with
	 COND possibly false, so the caller adds no "assume" hint).  */
    }

  /* if (!cond) reaction;  ==  cond ? (void) 0 : reaction.  */
  tree guard = build3 (COND_EXPR, void_type_node, cond, void_node, reaction);
  SET_EXPR_LOCATION (guard, loc);
  return guard;
}

/* LANG_HOOKS_BUILD_IMPLICIT_UB_HANDLER: build the pieces the middle end needs to
   call the contract-violation handler for an implicit UB assertion whose site
   (FNDECL + LOC) resolves to a *non-throwing* handler semantic
   (noexcept_enforce / noexcept_observe) for GROUP.  REACTION is the resolved
   enum implicit_ub_reaction carried from pass_ubsan (pre-inline, where the
   enclosing-function/namespace context was correct); we use it directly to pick
   the specific semantic instead of re-resolving against FNDECL, which after
   inlining is the caller and would no longer match the config -- the very bug
   that carrying the reaction operand exists to avoid.  On success returns true
   and sets *ENTRY_OUT to the __cxa_contract_violation entry-point FUNCTION_DECL
   and *DATA_ADDR_OUT to the address of a freshly-built static contract_violation
   data block for this site; the caller emits `entry (data_addr)` as a GIMPLE
   call.  The entry decl encodes noreturn for enforce and returns for observe,
   and is nothrow, so the call needs no EH region.  Returns false when REACTION
   is not a non-throwing handler reaction (the caller then handles it).  LOC (the
   inline-stable spelling location) supplies the data-block location; GROUP
   supplies the comment.

   This runs from the middle end, i.e. after free_lang_data; it only builds new
   trees (like the sanitizer's own ubsan_create_data) and reads GC-rooted
   descriptor tables, so it touches no freed language data.  */

bool
cp_build_implicit_ub_handler (tree fndecl, location_t loc, const char *group,
			      int reaction, tree *entry_out, tree *data_addr_out)
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

  /* Synthesize a contract node carrying the site location and a comment, then
     reuse the front-end data-block builders to emit the static
     contract_violation object (basic 8-field block -- implicit assertions have
     no label).  */
  tree contract = make_node (ASSERTION_STMT);
  TREE_TYPE (contract) = void_type_node;
  SET_EXPR_LOCATION (contract, loc);
  CONTRACT_COMMENT (contract) = build_string_literal (info.comment);

  tree block_type;
  tree ctor = build_contract_data_block_ctor (contract, &block_type);
  tree data_var = build_contract_data_block_constant (ctor, block_type,
						      contract);
  *data_addr_out = build_address (data_var);
  *entry_out = declare_cxa_entry_point (CAK_IMPLICIT, sem,
					CDM_PREDICATE_FALSE,
					/*is_noexcept=*/true);
  return true;
}

/* P3100: shared helper to guard a scalar operation OP_RESULT that is UB when
   COND holds, for the resolved semantic SEM (never CES_ASSUME).  Returns

     force, (cond ? <reaction-value> : op_result)

   FORCE is evaluated first so an operand referenced only on the ok-path (e.g. a
   division's dividend) still has its side effects on the violation path; all
   operands are SAVE_EXPRs by the time we get here, so each is evaluated once.
   On the violation path the UB operation is NOT executed:

     ignore              -> a defined (erroneous) zero;
     observe/nx_observe  -> call the handler, then a defined zero;
     quick_enforce       -> terminate;
     enforce/nx_enforce  -> call the noreturn handler.

   COMMENT is the contract-violation comment recorded for the check.  */

static tree
build_implicit_op_guard (location_t loc, contract_evaluation_semantic sem,
			 tree force, tree cond, tree op_result,
			 const char *comment)
{
  tree restype = TREE_TYPE (op_result);
  tree zero = build_zero_cst (restype);

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
      bool is_noexcept = (sem == CES_NOEXCEPT_ENFORCE
			  || sem == CES_NOEXCEPT_OBSERVE);
      tree contract = make_node (ASSERTION_STMT);
      TREE_TYPE (contract) = void_type_node;
      SET_EXPR_LOCATION (contract, loc);
      CONTRACT_COMMENT (contract) = build_string_literal (comment);

      tree block_type;
      tree ctor = build_contract_data_block_ctor (contract, &block_type);
      tree data_var = build_contract_data_block_constant (ctor, block_type,
							  contract);
      tree data_addr = build_address (data_var);
      tree entry = declare_cxa_entry_point (CAK_IMPLICIT, sem,
					    CDM_PREDICATE_FALSE, is_noexcept);
      tree call = build_call_n (entry, 1, data_addr);
      SET_EXPR_LOCATION (call, loc);
      viol_value = build2 (COMPOUND_EXPR, restype, call, zero);
    }

  tree guarded = build3_loc (loc, COND_EXPR, restype, cond, viol_value,
			     op_result);
  guarded = build2 (COMPOUND_EXPR, restype, force, guarded);
  return guarded;
}

/* P3100: guard an integer division/remainder DIV_RESULT (dividend OP0, divisor
   OP1) whose divisor may be zero -- core-language UB ({expr.mul.div.by.zero}).
   Returns `op0, (op1 == 0 ? <reaction> : div_result)`.  */

tree
build_implicit_divide_check (tree fndecl, location_t loc,
			     contract_evaluation_semantic sem,
			     tree op0, tree op1, tree div_result)
{
  gcc_checking_assert (sem != CES_ASSUME);
  (void) fndecl;

  tree cond = build2_loc (loc, EQ_EXPR, boolean_type_node,
			  op1, build_zero_cst (TREE_TYPE (op1)));
  return build_implicit_op_guard (loc, sem, op0, cond, div_result,
				  "integer division by zero");
}

/* P3100: guard a signed integer division/remainder DIV_RESULT (OP0 / OP1) whose
   quotient is not representable -- i.e. OP0 == the type minimum and OP1 == -1 --
   core-language UB ({expr.mul.representable}).  Returns
   `op0, (op0 == MIN && op1 == -1 ? <reaction> : div_result)`.  */

tree
build_implicit_divide_overflow_check (tree fndecl, location_t loc,
				      contract_evaluation_semantic sem,
				      tree op0, tree op1, tree div_result)
{
  gcc_checking_assert (sem != CES_ASSUME);
  (void) fndecl;

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
build_implicit_shift_check (tree fndecl, location_t loc,
			    contract_evaluation_semantic sem,
			    tree op0, tree op1, tree shift_result)
{
  gcc_checking_assert (sem != CES_ASSUME);
  (void) fndecl;

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
   array-bounds guard for a subscript at site (FNDECL + LOC) with a statically-
   known array size.  INDEX is the (integer) subscript; BOUND is the first
   out-of-range index value (the array size, already adjusted by the caller for a
   one-past address-of context).  Resolves the evaluation semantic for
   ub:expr.add.out.of.bounds.known and returns a replacement index expression
   `i, (out_of_range ? 0 : i)` -- redirecting an out-of-range subscript to the
   defined valid index 0 and running the configured reaction -- or NULL_TREE for
   the assume semantic (no guard, raw subscript).  Called from the c-family
   array-ref walk so the C++ reaction can be built without linking to cc1.  */

tree
cp_build_implicit_bounds_check (tree fndecl, location_t loc, tree index,
				tree bound)
{
  if (!flag_contracts_p3100 || fndecl == NULL_TREE)
    return NULL_TREE;

  contract_evaluation_semantic sem
    = resolve_implicit_contract_semantic (fndecl, loc,
					  "ub:expr.add.out.of.bounds.known");
  if (sem == CES_ASSUME)
    return NULL_TREE;

  /* Evaluate the index once and test it against the bound as unsigned, so a
     negative index (which wraps to a large value) is caught as well.  */
  tree i = save_expr (index);
  tree utype = unsigned_type_for (TREE_TYPE (i));
  tree cond = build2_loc (loc, GE_EXPR, boolean_type_node,
			  fold_convert (utype, i), fold_convert (utype, bound));
  return build_implicit_op_guard (loc, sem, i, cond, i,
				  "array subscript out of bounds");
}

/* P3100: guard a floating-point-to-integer conversion CONVERTED (the FIX_TRUNC
   of the floating value EXPR) whose truncated value may not be representable in
   the destination integer type -- core-language UB ({conv.fpint}).  EXPR must
   already be a single-evaluation form (SAVE_EXPR) shared with CONVERTED.
   Returns `expr, (out_of_range ? <reaction> : converted)`; if the conversion
   can never be out of range the CONVERTED value is returned unchanged.  */

tree
build_implicit_float_cast_check (tree fndecl, location_t loc,
				 contract_evaluation_semantic sem,
				 tree expr, tree converted)
{
  gcc_checking_assert (sem != CES_ASSUME);
  (void) fndecl;

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

tree
build_implicit_enum_cast_check (tree fndecl, location_t loc,
				contract_evaluation_semantic sem,
				tree expr, tree converted, tree enumtype)
{
  gcc_checking_assert (sem != CES_ASSUME);
  (void) fndecl;

  /* The enumeration's [dcl.enum] value range is carried by
     TREE_TYPE (enumtype), an INTEGER_TYPE.  Only values outside [min,max] are
     UB; if that range covers the full storage mode there is nothing to
     check.  */
  tree range_type = TREE_TYPE (enumtype);
  if (range_type == NULL_TREE
      || TREE_CODE (range_type) != INTEGER_TYPE
      || TYPE_MIN_VALUE (range_type) == NULL_TREE
      || TYPE_MAX_VALUE (range_type) == NULL_TREE
      || !(TYPE_PRECISION (range_type)
	   < GET_MODE_PRECISION (SCALAR_INT_TYPE_MODE (enumtype))))
    return converted;

  tree minv = TYPE_MIN_VALUE (range_type);
  tree maxv = TYPE_MAX_VALUE (range_type);

  /* out_of_range := (unsigned)(expr - minv) > (maxv - minv), computed in an
     unsigned type as wide as the source so a below-min value wraps to a large
     unsigned and is caught too (mirrors the load-site enum check).  */
  tree utype = unsigned_type_for (TREE_TYPE (expr));
  tree off = fold_build2_loc (loc, MINUS_EXPR, utype,
			      fold_convert (utype, expr),
			      fold_convert (utype, minv));
  tree span = fold_convert (utype, int_const_binop (MINUS_EXPR, maxv, minv));
  tree cond = build2_loc (loc, GT_EXPR, boolean_type_node, off, span);
  return build_implicit_op_guard (loc, sem, expr, cond, converted,
				  "enumeration value out of range");
}

/* Accessors for the cached P3595 dynamic-selector descriptor stored in
   CONTRACT_DYNAMIC by ensure_evaluation_semantic(..., in_ce=false).  */

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
  return (unsigned char) (tree_to_uhwi (TREE_VALUE (d)) >> 1);
}

static bool
contract_dynamic_provideweak (const_tree contract)
{
  tree d = CONTRACT_DYNAMIC (contract);
  gcc_checking_assert (d);
  return (tree_to_uhwi (TREE_VALUE (d)) & 1) != 0;
}

/* The return type of a dynamic-selection function: the real
   std::contracts::evaluation_semantic when the header is in scope, else
   the ABI-compatible uint16.  The return type is not part of the mangled
   name, so either choice binds to the same symbol.  */

static tree
dynamic_selector_return_type ()
{
  tree t = lookup_std_contracts_type (get_identifier ("evaluation_semantic"));
  if (t && t != error_mark_node && TREE_CODE (t) == ENUMERAL_TYPE)
    return t;
  return short_unsigned_type_node;
}

/* Map from selector IDENTIFIER_NODE to its FUNCTION_DECL, so we build at
   most one decl per unique name per TU.  */
static GTY(()) hash_map<tree, tree> *dynamic_selector_decls;

/* Selectors whose weak definition must be emitted at end of TU.  Each
   element is a TREE_LIST: PURPOSE=FUNCTION_DECL, VALUE=INTEGER_CST default
   semantic.  */
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
  /* This resolves the qualified name relative to current_namespace (via
     push_namespace, which starts from the current scope).  That is only
     correct because build_contract_check runs at genericization time, where
     the namespace scope has been unwound to the global namespace.  Make that
     invariant explicit: a stale non-global current_namespace would resolve or
     synthesize the selector in the wrong namespace and mismangle the symbol.  */
  gcc_checking_assert (current_namespace == global_namespace);

  const char *sep = strstr (name, "::");
  if (!sep)
    {
      /* Bare identifier: global namespace.  */
      *fn_id = get_identifier (name);
      return global_namespace;
    }

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

  return ns;
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
   PROVIDEWEAK, schedule a weak definition returning DEF_SEM to be emitted
   once for this name at end of TU.  */

static tree
get_dynamic_selector_decl (const char *name, unsigned char linkage,
			   bool provideweak,
			   contract_evaluation_semantic def_sem)
{
  /* Key the cache by the full NAME string: distinct qualified names (or
     distinct verbatim C symbols) map to distinct decls.  */
  tree key = get_identifier (name);

  if (!dynamic_selector_decls)
    dynamic_selector_decls = hash_map<tree, tree>::create_ggc (8);

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
	 C++ language so the symbol mangles normally.  */
      tree fn_id;
      tree ns = resolve_dynamic_selector_namespace (name, &fn_id);
      fndecl = build_lang_decl_loc (BUILTINS_LOCATION, FUNCTION_DECL,
				    fn_id, fntype);
      DECL_CONTEXT (fndecl) = FROB_CONTEXT (ns);
      SET_DECL_LANGUAGE (fndecl, lang_cplusplus);
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
   weak definition simply returns the entry's compile-time default
   semantic, so a program links and runs with no user-supplied selector,
   while a strong user definition overrides it at link time.  */

static void
emit_pending_weak_selectors ()
{
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
	 we no longer depend on source-level (namespace-scoped) name lookup.  */
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

/* Compute T(RAW) at compile time for the P3595 dynamic-dispatch transform:
   clamp RAW to the label's allowed set via the resolution fallback order, then
   apply the compute_semantic facet.  Sets *OK to false (and returns
   CES_INVALID) when the clamp finds no allowed semantic or the compute_semantic
   result is disallowed -- stage 2 turns that sentinel into a runtime enforced
   violation.  This mirrors clamp_semantic_to_allowed + apply_compute_semantic
   in contracts-config.cc / this file, but never issues a compile-time error.  */

static contract_evaluation_semantic
transform_semantic (tree contract, tree fndecl,
		    contract_evaluation_semantic raw, bool *ok)
{
  contract_query q = make_contract_query (contract, fndecl);
  uint16_t mask = q.allowed_mask;

  /* Stage: clamp to the allowed set using the best-fit safety-level search.  */
  uint16_t s
    = (uint16_t) contract_semantic_best_fit ((contract_evaluation_semantic) raw,
					     mask);

  /* Stage: apply compute_semantic (returns CES_INVALID if disallowed).  */
  if (s != CES_INVALID)
    s = apply_compute_semantic_value (CONTRACT_LABEL (contract), s, mask);

  *ok = (s != CES_INVALID) && (mask & (1 << s)) != 0;
  return (contract_evaluation_semantic) (*ok ? s : (uint16_t) CES_INVALID);
}

/* Does CONTRACT's label transform the raw selector value non-trivially?
   True when a compute_semantic facet is present or allowed_semantics narrows
   the standard four-semantic set -- in which case the P3595 dynamic path must
   emit the two-stage map/dispatch.  When false the map is the identity and the
   single-stage cascade is used.  */

static bool
contract_label_transforms_p (tree contract, tree fndecl)
{
  tree label = CONTRACT_LABEL (contract);
  if (label_has_compute_semantic (label))
    return true;
  /* allowed_semantics narrows the set iff the query's allowed_mask drops any
     of the standard four semantics.  (The -fcontracts-allow-assume "assume"
     bit is never returnable by a conforming selector, so it is irrelevant.)  */
  contract_query q = make_contract_query (contract, fndecl);
  return (q.allowed_mask & CES_ALL_ALLOWED) != CES_ALL_ALLOWED;
}

/* Build the contract check (new ABI version).
   This is called during genericization.  */

tree
build_contract_check (tree contract)
{
  contract_evaluation_semantic semantic
    = ensure_evaluation_semantic (contract, current_function_decl, false);

  /* Plain (non-dynamic) contract: emit the single resolved check.  */
  const char *dyn_name = contract_dynamic_name (contract);
  if (!dyn_name)
    return emit_check_for_semantic (contract, semantic);

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
     present), a TWO-STAGE form is emitted (P3595 design 4):

       stage 1: eff = T(raw), mapping each of ignore/observe/enforce/quick
		to its compile-time transform T() (or CES_INVALID when the
		result is disallowed); an unknown raw value maps to CES_INVALID.
       stage 2: dispatch on eff, calling emit_check_for_semantic for the four
		valid semantics and emit_enforced_violation for CES_INVALID.

     When the map is the identity (no transforming label) stage 1 is skipped and
     stage 2 dispatches directly on the raw selector value.  */
  bool provideweak = contract_dynamic_provideweak (contract);
  unsigned char linkage = contract_dynamic_linkage (contract);

  tree fndecl = get_dynamic_selector_decl (dyn_name, linkage, provideweak,
					   semantic);
  tree ret_type = TREE_TYPE (TREE_TYPE (fndecl));

  tree cc_bind = build3 (BIND_EXPR, void_type_node, NULL, NULL, NULL);
  BIND_EXPR_BODY (cc_bind) = push_stmt_list ();

  /* The violation data block is identical for every dispatch arm of this
     contract (same source location, comment, kind, ...), so build it ONCE here
     and reuse its address across all arms and the enforced-violation default,
     rather than emitting a duplicate global per arm.  */
  tree block_type;
  tree ctor = build_contract_data_block_ctor (contract, &block_type);
  tree data_var = build_contract_data_block_constant (ctor, block_type,
						      contract);
  tree data_addr = build_address (data_var);

  tree call = build_call_n (fndecl, 0);
  tree raw = save_expr (call);

  bool transforms
    = contract_label_transforms_p (contract, current_function_decl);

  /* The value stage 2 dispatches on: the raw selector value for the identity
     map, or the transformed "eff" temporary for the two-stage form.  */
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

      auto_vec<tree, 4> map_ifs;
      for (int s = CES_IGNORE; s <= CES_QUICK; s++)
	{
	  tree cmp = build2 (EQ_EXPR, boolean_type_node, raw,
			     build_int_cst (ret_type, s));
	  tree if_stmt = begin_if_stmt ();
	  finish_if_stmt_cond (cmp, if_stmt);
	  bool ok = false;
	  contract_evaluation_semantic eff_sem
	    = transform_semantic (contract, current_function_decl,
				  (contract_evaluation_semantic) s, &ok);
	  /* A dynamically-resolved "assume" cannot inform the optimizer -- the
	     predicate is never evaluated on this path, so there is nothing to
	     assume from -- and the only universally-correct behavior is to do
	     exactly what "ignore" does: no check, no violation, continue.  Map
	     it to CES_IGNORE so stage 2's ignore arm handles it.  This is only
	     reachable when -fcontracts-allow-assume put assume in the allowed
	     set (a valid semantic choice); without the flag assume is not
	     allowed, so ok is false and eff becomes CES_INVALID below, driving
	     an enforced violation for the broken configuration.  */
	  int eff_val = !ok ? (int) CES_INVALID
		      : eff_sem == CES_ASSUME ? (int) CES_IGNORE
		      : (int) eff_sem;
	  finish_expr_stmt
	    (cp_build_modify_expr (loc, eff, NOP_EXPR,
				   build_int_cst (short_unsigned_type_node,
						  eff_val),
				   tf_warning_or_error));
	  finish_then_clause (if_stmt);
	  begin_else_clause (if_stmt);
	  map_ifs.safe_push (if_stmt);
	}
      /* Default (unknown raw value): eff = CES_INVALID.  */
      finish_expr_stmt
	(cp_build_modify_expr (loc, eff, NOP_EXPR,
			       build_int_cst (short_unsigned_type_node,
					      (int) CES_INVALID),
			       tf_warning_or_error));
      for (int i = map_ifs.length () - 1; i >= 0; i--)
	{
	  finish_else_clause (map_ifs[i]);
	  finish_if_stmt (map_ifs[i]);
	}

      dispatch_val = eff;
      dispatch_type = short_unsigned_type_node;
    }

  /* Stage 2 (or the sole stage for the identity map): dispatch on
     DISPATCH_VAL, emitting each distinct check body once.  Nested
     if-statements, opened outermost-first and closed innermost-first so that
     their else clauses nest into the enforced-violation default.  */
  auto_vec<tree, 4> if_stmts;
  for (int s = CES_IGNORE; s <= CES_QUICK; s++)
    {
      tree cmp = build2 (EQ_EXPR, boolean_type_node, dispatch_val,
			 build_int_cst (dispatch_type, s));
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

  /* Final else: an unknown value (identity map) or the CES_INVALID sentinel
     (two-stage map) yields an enforced violation.  */
  tree def_body = emit_enforced_violation (contract, data_addr);
  if (def_body && def_body != error_mark_node)
    add_stmt (def_body);

  /* Close the else clauses / if statements, innermost first.  */
  for (int i = if_stmts.length () - 1; i >= 0; i--)
    {
      finish_else_clause (if_stmts[i]);
      finish_if_stmt (if_stmts[i]);
    }

  BIND_EXPR_BODY (cc_bind) = pop_stmt_list (BIND_EXPR_BODY (cc_bind));
  return cc_bind;
}

#include "gt-cp-contracts.h"
