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

/* Lookup a name in std::, or inject it.  */

static tree
lookup_std_type (tree name_id)
{
  tree res_type = lookup_qualified_name
    (std_node, name_id, LOOK_want::TYPE | LOOK_want::HIDDEN_FRIEND);

  if (TREE_CODE (res_type) == TYPE_DECL)
    res_type = TREE_TYPE (res_type);
  else
    {
      push_nested_namespace (std_node);
      res_type = make_class_type (RECORD_TYPE);
      create_implicit_typedef (name_id, res_type);
      DECL_SOURCE_LOCATION (TYPE_NAME (res_type)) = BUILTINS_LOCATION;
      DECL_CONTEXT (TYPE_NAME (res_type)) = current_namespace;
      pushdecl_namespace_level (TYPE_NAME (res_type), /*hidden*/true);
      pop_nested_namespace (std_node);
    }
  return res_type;
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

/* Get contract_evaluation_semantic of the specified contract.  */

contract_evaluation_semantic
get_evaluation_semantic (const_tree contract)
{
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
  if (CONTRACT_EVALUATION_SEMANTIC (contract))
    {
      tree s = CONTRACT_EVALUATION_SEMANTIC (contract);
      tree i = (TREE_CODE (s) == INTEGER_CST) ? s
					      : DECL_INITIAL (STRIP_NOPS (s));
      gcc_checking_assert (!type_dependent_expression_p (s) && i);
      switch (contract_evaluation_semantic ev =
	      (contract_evaluation_semantic) tree_to_uhwi (i))
	{
	/* This needs to be kept in step with any added semantics.  */
	case CES_IGNORE:
	case CES_OBSERVE:
	case CES_ENFORCE:
	case CES_QUICK:
	  return ev;
	default:
	  break;
	}
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
  gcc_unreachable ();
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
   predicate pushes its own scopes -- which left a variable named there
   unconstified even though [expr.prim.id.unqual]/3+d covers it.  The
   paragraph's own example is explicit about this: given a namespace-scope
   `int n`, `pre([=,&i,*this] mutable { ++n; ... }())` marks `++n` an error.

   This is deliberately additive: when the innermost level IS the contract
   scope, nothing here runs and the existing path is unchanged.

   Two exemptions keep the rest of that example working:

     * A lambda CAPTURE PROXY is not the variable; the id-expression denotes a
       member of the closure type, which the example marks OK (`++p`, `++r`).
       A captured entity that must be const already is -- the capture
       initializer was constified out in the enclosing predicate, which is why
       `++i` on a by-reference capture is an error for its own reason.
     * An entity DECLARED INSIDE the predicate -- a local of the lambda -- is
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
  /* We check if we have a variable, a parameter, a variable of reference type,
   * or a parameter of reference type
   */
  if (!TREE_READONLY (decl)
      && (VAR_P (decl)
	  || (TREE_CODE (decl) == PARM_DECL)
	  || (REFERENCE_REF_P (decl)
	      && (VAR_P (TREE_OPERAND (decl, 0))
		  || (TREE_CODE (TREE_OPERAND (decl, 0)) == PARM_DECL)
		  || (TREE_CODE (TREE_OPERAND (decl, 0))
		      == TEMPLATE_PARM_INDEX)))))
    decl = view_as_const (decl);

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
};

/* cp_walk_tree callback implementing [dcl.contract.func]/7 over a FINISHED
   postcondition predicate: "If the predicate of a postcondition assertion of
   a function f odr-uses a non-reference parameter of f, that parameter ...
   shall have const type."  */

static tree check_postcondition_odr_use_r (tree *, int *, void *);

/* E is a discarded-value expression inside a postcondition predicate.  Its
   POTENTIAL RESULTS ([basic.def.odr]/2) are not odr-uses, so a parameter that
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
     NOP_EXPR is deliberately NOT stripped: an ordinary conversion applies the
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

    default:
      break;
    }

  cp_walk_tree_without_duplicates (&e, check_postcondition_odr_use_r, d);
}

static tree
check_postcondition_odr_use_r (tree *tp, int *walk_subtrees, void *data)
{
  tree t = *tp;
  auto *d = (postcondition_odr_use_data *) data;

  /* An unevaluated operand is not an odr-use.  Most of these have already
     folded away by now, but a dependent one has not.

     The set to stop at is not a judgement call: cp_walk_subtrees enters
     exactly three kinds of subtree under `cp_unevaluated`, and this switch
     covers all three.  Missing two of them rejected programs stock g++
     accepts -- a decltype whose operand is still dependent, and a
     requires-expression, which survives into the finished predicate whether
     or not anything in it is dependent.  */
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
       is actually odr-used, and the walk that runs then sees only that one.
       Descending here is what the old defer_postcondition_pack_index_check
       flag existed to suppress, element by element, from the middle of
       tsubst_pack_index.  */
    case PACK_INDEX_EXPR:
      *walk_subtrees = 0;
      return NULL_TREE;

    case COMPOUND_EXPR:
      {
	/* [expr.context]: the left operand of a comma is a discarded-value
	   expression, and the lvalue-to-rvalue conversion is not applied to
	   it unless it is volatile-qualified.  A parameter that IS that
	   operand is therefore not odr-used ([basic.def.odr]/5) -- this is
	   PR126897, `void f (bool b) post ((b, true)) {}`.

	   convert_to_void has usually already replaced a side-effect-free
	   discarded operand with void_node, in which case there is nothing
	   here to find; but it does not fold a DEPENDENT predicate, which is
	   what a postcondition with a result-name-introducer has while it is
	   being parsed.  So recognise the shape rather than relying on it.

	   What is exempt is the set of POTENTIAL RESULTS of the discarded
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

  /* [dcl.contract.func]/7 constrains a NON-REFERENCE parameter.  The old
     id-expression check got this for free, because finish_id_expression hands
     back an INDIRECT_REF for a reference parameter and never a bare
     PARM_DECL; a tree walk descends through that wrapper and reaches the
     PARM_DECL itself, so the reference case has to be excluded explicitly.  */
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
     when the contracted function IS a lambda, its own parameters still have
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
    }

  return NULL_TREE;
}

/* Apply [dcl.contract.func]/7 to the finished postcondition predicate
   CONDITION of FNDECL.

   This runs on the FINISHED expression rather than on each id-expression as
   it is parsed, because whether a parameter is odr-used is not known until
   the expression around it is.  The motivating case is PR126897:

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
   unexpanded pack.  */

void
check_postcondition_param_odr_uses (tree condition, tree fndecl,
				    location_t loc)
{
  if (!condition || condition == error_mark_node
      || TREE_CODE (condition) == DEFERRED_PARSE)
    return;

  postcondition_odr_use_data data = { fndecl, loc };
  cp_walk_tree_without_duplicates (&condition, check_postcondition_odr_use_r,
				   &data);
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

/* [dcl.contract.func]/6: a deleted function, or one defaulted on its first
   declaration, shall not have a function-contract-specifier-seq.  DECL has
   just been marked as one of those; DELETED_P says which.  Diagnose a
   contract on it and drop the contract, rather than silently ignoring it --
   such a function has no body the precondition could guard, so the contract
   was simply doing nothing.

   The paragraph's third case, a *virtual* function, is DELIBERATELY NOT
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

static GTY(()) hash_map<tree, tree> *postcondition_redecl_parms;

static void record_postcondition_redecl_parm (tree, unsigned, tree);

/* Carry the "odr used in a postcondition" property of the parameter T1 of
   OLDDECL over to the parameter T2 of a redeclaration or instantiation that
   corresponds to it, and check that T2 satisfies the const requirement.  */

static void
check_postcondition_parm_in_redecl (tree olddecl, tree t1, tree t2,
				    unsigned idx)
{
  if (!parm_used_in_post_p (t1))
    return;

  set_parm_used_in_post (t2);

  /* Either of these declarations may be the one duplicate_decls discards, and
     which it is depends on things this function cannot see (a definition's
     parameters win).  Record both: recording is idempotent, and a parameter
     that does survive is checked directly anyway, so a redundant record costs
     nothing.  Recording only T1 loses the middle declaration of three
     (PR c++/127196).  */
  record_postcondition_redecl_parm (olddecl, idx, t1);
  record_postcondition_redecl_parm (olddecl, idx, t2);

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

/* PR c++/127196.  Record, against the surviving FUNCTION_DECL OLDDECL, that
   the parameter PARM at index IDX of a declaration being merged away has a
   dependent type that is not const.  [dcl.contract.func]/7 requires the
   corresponding parameter on ALL declarations to be const once the predicate
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

  if (!postcondition_redecl_parms)
    postcondition_redecl_parms = hash_map<tree, tree>::create_ggc ();

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
   looks fine and an earlier declaration did not.  */

void
check_postcondition_redecl_parm_types (tree pattern, tree spec, tree args)
{
  if (!postcondition_redecl_parms)
    return;

  tree *slot = postcondition_redecl_parms->get (pattern);
  if (!slot)
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

      tree type = tsubst (TREE_TYPE (recorded), args, tf_none, NULL_TREE);
      if (type == error_mark_node || uses_template_parms (type)
	  || CP_TYPE_CONST_P (type))
	continue;

      auto_diagnostic_group d;
      error_at (DECL_SOURCE_LOCATION (sp),
		"value parameter %qE used in a postcondition must be const",
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
static GTY(()) hash_map<tree, tree> *decl_pre_fn;
static GTY(()) hash_map<tree, tree> *decl_post_fn;

/* Map from label type -> local violation handler trampoline FUNCTION_DECL.
   Generated at parse time, looked up during gimplification.  */
static GTY(()) hash_map<tree, tree> *local_violation_trampoline_map;

/* Label types already checked for near-miss facets, so that a label used on a
   hundred contracts does not warn a hundred times.  */
static GTY(()) hash_set<tree> *near_miss_checked_types;

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
  if (!has_active_preconditions (fndecl))
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
get_or_create_contract_wrapper_function (tree fndecl)
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
		/* ID is a location wrapper when one could be built, a DECL
		   on the paths that have already made the result variable,
		   and a bare IDENTIFIER_NODE otherwise -- which is what a
		   deferred contract carries.  DECL_SOURCE_LOCATION on an
		   identifier reads fields that are not there and yields a
		   garbage line number, so ask what ID is first.  */
		location_t id_l = UNKNOWN_LOCATION;
		if (location_wrapper_p (id))
		  id_l = EXPR_LOCATION (id);
		else if (DECL_P (id))
		  id_l = DECL_SOURCE_LOCATION (id);
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

  /* A postcondition check runs after the returned object has been
     initialized, and a violation handler that throws unwinds straight
     through it.  Record that as what it is -- a cleanup that might throw --
     so maybe_set_retval_sentinel builds the sentinel even for a function
     whose body has no throwing cleanup of its own, and so the one cleanup
     that results is spliced around the contracts block rather than the body.
     Without this the returned object leaks (PR c++/127414), and the two
     splices the contracts block used to provoke double-destroyed it
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
  /* If we're starting a guarded function with valid contracts, we need to
     insert a call to the pre function.  */
  gcc_checking_assert (DECL_PRE_FN (fndecl)
		       && DECL_PRE_FN (fndecl) != error_mark_node);

  releasing_vec args = build_arg_list (fndecl);
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

     - Returned in memory: DECL_RESULT is addressable and IS the object the
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
				   get_identifier ("__contract_post_retval"),
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

static GTY(()) hash_map<tree, tree> *postcondition_capture_inits;

/* Map from a guarded function -> the local standing in for its returned
   object in that function's inline postcondition checks.  A function is
   absent when its checks read DECL_RESULT directly; see
   postcondition_needs_retval_temp_p for when a temporary is needed.

   Populated by apply_postconditions and read by remap_retval, which does not
   run until genericization -- emit_contract_statement only queues the
   contract -- so this cannot be a per-function transient.  GC-managed for
   the same reason.  */

static GTY(()) hash_map<tree, tree> *postcondition_retval_temps;

/* Emit the initialization of one postcondition capture (P3098).  TARGET is
   the capture object -- a VAR_DECL on the inline path, a capture-struct
   member on the outlined one -- and INIT its initializer.

   A capture is a local variable copy-initialized from its initializer
   ([dcl.contract.capture]: the capture's initializer is a const lvalue
   denoting the parameter, or the init-capture's own initializer), so a class
   type has to go through build_aggr_init.  A bare INIT_EXPR performs no
   overload resolution: it bit-copies the object, and the capture's
   destructor then runs on a copy that no constructor ever made.  */

static GTY(()) hash_map<tree, tree> *postcondition_capture_struct_types;

/* Build (or return cached) capture-state struct type for a postcondition
   with captures.  The struct has a bool __initialized field followed by
   union-wrapped fields for each capture (unions prevent implicit
   construction/destruction).  */

/* Add a call or a direct evaluation of the pre checks.  */

static void
apply_preconditions (tree fndecl)
{
  if (flag_contract_checks_outlined)
    add_pre_condition_fn_call (fndecl);
  else
  {
    if (tree contract_copy = copy_contracts (fndecl, cmk_pre))
      for (tree contract : tree_vec_range (contract_copy))
	emit_contract_statement (contract);
  }
}

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
     to a temporary of its own EVERY TIME a predicate takes its address -- and
     a predicate may do so more than once:

       int f () post (r : rec (&r) && rec2 (addr_via_ref (r))) ...

     gives two spills and therefore two addresses for one result binding,
     inside a single evaluation of a single predicate.  [dcl.contract.res]/1
     binds the result name to one object; nothing permits two, and the
     predicate's value comes out wrong as a result (PR112794).  Give it one
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
    if (TREE_CODE (contract) == POSTCONDITION_STMT)
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
/* Add a call or a direct evaluation of the post checks.  */

static void
apply_postconditions (tree fndecl)
{
  if (flag_contract_checks_outlined && DECL_POST_FN (fndecl))
    {
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
      if (!postcondition_retval_temps)
	postcondition_retval_temps = hash_map<tree, tree>::create_ggc ();
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

  /* If this is not a client side check and definition side checks are
     disabled, do nothing.  */
  if (!flag_contracts_definition_check
      && !DECL_CONTRACT_WRAPPER (fndecl))
    return;

  bool do_pre = has_active_preconditions (fndecl);
  bool do_post = has_active_postconditions (fndecl);
  /* We should not have reached here with nothing to do... */
  gcc_checking_assert (do_pre || do_post);

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

  /* If we have a pre, but not a post, then just emit that and we are done.  */
  if (!do_post)
    {
      apply_preconditions (fndecl);
      add_stmt (fnbody);
      finish_compound_stmt (compound_stmt);
      return;
    }

  if (do_pre)
    /* Add a precondition call, if we have one. */
    apply_preconditions (fndecl);
  tree try_fin = build_stmt (loc, TRY_FINALLY_EXPR, fnbody, NULL_TREE);
  add_stmt (try_fin);
  TREE_OPERAND (try_fin, 1) = push_stmt_list ();
  /* If we have exceptions, and a function that might throw, then add
     an EH_ELSE clause that allows the exception to propagate upwards
     without encountering the post-condition checks.  */
  if (flag_exceptions && !type_noexcept_p (TREE_TYPE (fndecl)))
    {
      tree eh_else = build_stmt (loc, EH_ELSE_EXPR, NULL_TREE, NULL_TREE);
      add_stmt (eh_else);
      TREE_OPERAND (eh_else, 0) = push_stmt_list ();
      apply_postconditions (fndecl);
      TREE_OPERAND (eh_else, 0) = pop_stmt_list (TREE_OPERAND (eh_else, 0));
      TREE_OPERAND (eh_else, 1) = void_node;
    }
  else
    apply_postconditions (fndecl);
  TREE_OPERAND (try_fin, 1) = pop_stmt_list (TREE_OPERAND (try_fin, 1));

  /* Hand the splice back to do_poplevel: everything the cleanup must cover is
     now in this block, so closing it is what puts the cleanup in the right
     place.  */
  cp_function_chain->defer_retval_cleanup = false;
  finish_compound_stmt (compound_stmt);
  /* The DECL_SAVED_TREE stmt list will be popped by our caller.  */
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
      if (!sp && dp
	  && TREE_CODE (contract) == POSTCONDITION_STMT
	  && DECL_CHAIN (dp) == NULL_TREE)
	{
	  gcc_assert (!duplicate_p);
	  if (tree result = POSTCONDITION_IDENTIFIER (contract))
	    {
	      gcc_assert (DECL_P (result));
	      insert_decl_map (&id, result, dp);
	      do_remap = true;
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

  auto_vec<tree> copies (TREE_VEC_LENGTH (contracts));
  for (tree contract : tree_vec_range (contracts))
    {
      if ((remap_kind == cmk_pre
	   && TREE_CODE (contract) == POSTCONDITION_STMT)
	  || (remap_kind == cmk_post
	      && TREE_CODE (contract) == PRECONDITION_STMT))
	continue;

      tree stmt = copy_node (contract);

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
	 flush_deferred_contract_matches once both have been.

	 This used to do nothing at all, which was survivable only while the
	 case was confined to friend declarations -- their contracts are
	 late-parsed at the end of the class while the same function declared
	 outside is not.  Now that every function contract is deferred to its
	 late parse, doing nothing here would skip redeclaration matching
	 entirely and accept any mismatch in silence.  */
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
maybe_contract_wrap_call (tree fndecl, tree call)
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
  /* We check postconditions if postcondition checks are enabled for clients.
    We should not get here unless there are some checks to make.  */
  bool check_post = flag_contract_client_check > 1;
  /* For wrappers on CDTORs we need to refer to the original contracts,
     when the wrapper is around a clone.  */
  set_fn_contract_specifiers ( wrapdecl,
		      copy_and_remap_contracts (wrapdecl, DECL_ORIGIN (fndecl),
						check_post? cmk_all : cmk_pre));

  start_preparsed_function (wrapdecl, /*DECL_ATTRIBUTES*/NULL_TREE,
			    SF_DEFAULT | SF_PRE_PARSED);
  tree body = begin_function_body ();
  tree compound_stmt = begin_compound_stmt (BCS_FN_BODY);

  vec<tree, va_gc> * args = build_arg_list (wrapdecl);

  /* We do not support contracts on virtual functions yet.  */
  gcc_checking_assert (!DECL_IOBJ_MEMBER_FUNCTION_P (fndecl)
		       || !DECL_VIRTUAL_P (fndecl));

  tree call = build_thunk_like_call (fndecl, args->length (), args->address ());

  finish_return_stmt (call);

  finish_compound_stmt (compound_stmt);
  finish_function_body (body);
  expand_or_defer_fn (finish_function (/*inline_p=*/false));
  return true;
}

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

tree
grok_contract (tree contract_spec, tree mode, tree result, cp_expr condition,
	       location_t loc)
{
  if (condition == error_mark_node)
    return error_mark_node;

  tree_code code;
  contract_assertion_kind kind = CAK_INVALID;
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
    contract = build5_loc (loc, code, void_type_node, mode,
			   NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE);
  else
    {
      contract = build_nt (code, mode, NULL_TREE, NULL_TREE,
			   NULL_TREE, NULL_TREE, result);
      TREE_TYPE (contract) = void_type_node;
      SET_EXPR_LOCATION (contract, loc);
    }

  /* Determine the assertion kind.  */
  CONTRACT_ASSERTION_KIND (contract) = build_int_cst (uint16_type_node, kind);

  /* Determine the evaluation semantic.  This is now an override, so that if
     not set we will get the default (currently enforce).  */
  CONTRACT_EVALUATION_SEMANTIC (contract)
    = build_int_cst (uint16_type_node, (uint16_t)
		     flag_contract_evaluation_semantic);

  /* If the contract is deferred, don't do anything with the condition.  */
  if (TREE_CODE (condition) == DEFERRED_PARSE)
    {
      CONTRACT_CONDITION (contract) = condition;
      return contract;
    }

  /* Generate the comment from the original condition.  */
  CONTRACT_COMMENT (contract) = build_comment (condition);

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
update_late_contract (tree contract, tree fndecl, tree result,
		      cp_expr condition)
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

  /* The predicate is complete, so [dcl.contract.func]/7 can be applied to it.
     This is the one place every late-parsed function contract passes through,
     which is what makes it the right hook: a postcondition with no
     result-name-introducer never reaches the result-variable rebuild.  */
  if (POSTCONDITION_P (contract))
    check_postcondition_param_odr_uses (condition, fndecl,
					EXPR_LOCATION (contract));
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

  /* If this is not a client side check and definition side checks are
     disabled, do nothing.  */
  if (!flag_contracts_definition_check
      && !DECL_CONTRACT_WRAPPER (fndecl))
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

  if (pre && !DECL_INITIAL (pre))
    {
      DECL_PENDING_INLINE_P (pre) = false;
      start_preparsed_function (pre, DECL_ATTRIBUTES (pre), flags);
      remap_and_emit_conditions (fndecl, pre, PRECONDITION_STMT);
      finish_return_stmt (NULL_TREE);
      pre = finish_function (false);
      expand_or_defer_fn (pre);
    }

  if (post && !DECL_INITIAL (post))
    {
      DECL_PENDING_INLINE_P (post) = false;
      start_preparsed_function (post, DECL_ATTRIBUTES (post), flags);
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
declare_one_violation_handler_wrapper (tree fn_name, tree fn_type,
				       tree p1_type, tree p2_type)
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
build_terminate_wrapper ()
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
build_contract_handler_call (tree violation)
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
  init_builtin_contract_violation_type ();
}

static GTY(()) tree contracts_source_location_impl_type;

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
  if (contracts_source_location_impl_type)
     return contracts_source_location_impl_type;

  /* First see if we have a declaration that we can use.  */
  tree contracts_source_location_type
    = lookup_std_type (get_identifier ("source_location"));

  if (contracts_source_location_type
      && contracts_source_location_type != error_mark_node
      && TYPE_FIELDS (contracts_source_location_type))
    {
      contracts_source_location_impl_type = get_source_location_impl_type ();
      return contracts_source_location_impl_type;
    }

  /* We do not, so build the __impl layout equivalent type, which must
     match <source_location>:
     struct __impl
      {
	  const char* _M_file_name;
	  const char* _M_function_name;
	  unsigned _M_line;
	  unsigned _M_column;
      }; */
  const tree types[] = { const_string_type_node,
			const_string_type_node,
			uint_least32_type_node,
			uint_least32_type_node };

 const char *names[] = { "_M_file_name",
			 "_M_function_name",
			 "_M_line",
			 "_M_column",
			};
  tree fields = NULL_TREE;
  unsigned n = 0;
  for (tree type : types)
  {
    /* finish_builtin_struct wants fields chained in reverse.  */
    tree next = build_decl (BUILTINS_LOCATION, FIELD_DECL,
			    get_identifier (names[n++]), type);
    DECL_CHAIN (next) = fields;
    fields = next;
  }

  iloc_sentinel ils (input_location);
  input_location = BUILTINS_LOCATION;
  contracts_source_location_impl_type = cxx_make_type (RECORD_TYPE);
  finish_builtin_struct (contracts_source_location_impl_type,
			 "__impl", fields, NULL_TREE);
  DECL_CONTEXT (TYPE_NAME (contracts_source_location_impl_type)) = context;
  DECL_ARTIFICIAL (TYPE_NAME (contracts_source_location_impl_type)) = true;
  TYPE_ARTIFICIAL (contracts_source_location_impl_type) = true;
  contracts_source_location_impl_type
    = cp_build_qualified_type (contracts_source_location_impl_type,
			       TYPE_QUAL_CONST);

  return contracts_source_location_impl_type;
}

static tree
get_src_loc_impl_ptr (location_t loc)
{
  if (!contracts_source_location_impl_type)
    get_contracts_source_location_impl_type ();

  tree fndecl = current_function_decl;
  /* We might be an outlined function.  */
  if (DECL_IS_PRE_FN_P (fndecl) || DECL_IS_POST_FN_P (fndecl))
    fndecl = get_orig_for_outlined (fndecl);
  /* We might be a wrapper.  */
  if (DECL_IS_WRAPPER_FN_P (fndecl))
    fndecl = get_orig_func_for_wrapper (fndecl);

  gcc_checking_assert (fndecl);
  tree impl__
    = build_source_location_impl (loc, fndecl,
				  contracts_source_location_impl_type);
  tree p = build_pointer_type (contracts_source_location_impl_type);
  return build_fold_addr_expr_with_type_loc (loc, impl__, p);
}

/* Build a contract_violation layout compatible object. */

/* Constructor.  At present, this should always be constant. */

static tree
build_contract_violation_ctor (tree contract)
{
  bool can_be_const = true;
  uint16_t version = 1;
  /* Default CDM_PREDICATE_FALSE. */
  uint16_t detection_mode = CDM_PREDICATE_FALSE;

  tree assertion_kind = CONTRACT_ASSERTION_KIND (contract);
  if (!assertion_kind || really_constant_p (assertion_kind))
    {
      contract_assertion_kind kind = get_contract_assertion_kind (contract);
      assertion_kind = build_int_cst (uint16_type_node, kind);
    }
  else
    can_be_const = false;

  tree eval_semantic = CONTRACT_EVALUATION_SEMANTIC (contract);
  gcc_checking_assert (eval_semantic);
  if (!really_constant_p (eval_semantic))
    can_be_const = false;

  tree comment = CONTRACT_COMMENT (contract);
  if (comment && !really_constant_p (comment))
    can_be_const = false;

  tree std_src_loc_impl_ptr = CONTRACT_STD_SOURCE_LOC (contract);
  if (std_src_loc_impl_ptr)
    {
      std_src_loc_impl_ptr = convert_from_reference (std_src_loc_impl_ptr);
      if (!really_constant_p (std_src_loc_impl_ptr))
	can_be_const = false;
    }
  else
    std_src_loc_impl_ptr = get_src_loc_impl_ptr (EXPR_LOCATION (contract));

  /* Must match the type layout in builtin_contract_violation_type.  */
  tree f0 = next_aggregate_field (TYPE_FIELDS (builtin_contract_violation_type));
  tree f1 = next_aggregate_field (DECL_CHAIN (f0));
  tree f2 = next_aggregate_field (DECL_CHAIN (f1));
  tree f3 = next_aggregate_field (DECL_CHAIN (f2));
  tree f4 = next_aggregate_field (DECL_CHAIN (f3));
  tree f5 = next_aggregate_field (DECL_CHAIN (f4));
  tree f6 = next_aggregate_field (DECL_CHAIN (f5));
  tree ctor = build_constructor_va
    (builtin_contract_violation_type, 7,
     f0, build_int_cst (uint16_type_node, version),
     f1, assertion_kind,
     f2, eval_semantic,
     f3, build_int_cst (uint16_type_node, detection_mode),
     f4, comment,
     f5, std_src_loc_impl_ptr,
     f6, build_zero_cst (nullptr_type_node)); // __vendor_ext

  TREE_READONLY (ctor) = true;
  if (can_be_const)
    TREE_CONSTANT (ctor) = true;

  return ctor;
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
  /* Compiler-generated.  */
  DECL_ARTIFICIAL (var_) = true;
  TREE_CONSTANT (var_) = true;
  layout_decl (var_, 0);
  return var_;
}

/* Create a read-only violation object.  */

static tree
build_contract_violation_constant (tree ctor, tree contract)
{
  tree viol_ = contracts_tu_local_named_var
    (EXPR_LOCATION (contract), "Lcontract_violation",
     builtin_contract_violation_type);

  TREE_CONSTANT (viol_) = true;
  DECL_INITIAL (viol_) = ctor;
  varpool_node::finalize_decl (viol_);

  return viol_;
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

  tree ctor = build_constructor_va
    (d3, 8,
     d3_fields[0], build_int_cst (unsigned_char_type_node, CXA_DESC_HEADER_BYTE),
     d3_fields[1], build_int_cst (unsigned_char_type_node, 3),
     d3_fields[2], build_int_cst (unsigned_char_type_node, CXA_FIELD_SOURCE_LOCATION),
     d3_fields[3], build_int_cst (unsigned_char_type_node, CXA_FIELD_COMMENT),
     d3_fields[4], build_int_cst (unsigned_char_type_node, CXA_FIELD_MESSAGE),
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
     d5_fields[0], build_int_cst (unsigned_char_type_node, CXA_DESC_HEADER_BYTE),
     d5_fields[1], build_int_cst (unsigned_char_type_node, 5),
     d5_fields[2], build_int_cst (unsigned_char_type_node, CXA_FIELD_SOURCE_LOCATION),
     d5_fields[3], build_int_cst (unsigned_char_type_node, CXA_FIELD_COMMENT),
     d5_fields[4], build_int_cst (unsigned_char_type_node, CXA_FIELD_MESSAGE),
     d5_fields[5], build_int_cst (unsigned_char_type_node, CXA_FIELD_LOCAL_HANDLER),
     d5_fields[6], build_int_cst (unsigned_char_type_node, CXA_FIELD_LABEL_PTR),
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
     dq_fields[0], build_int_cst (unsigned_char_type_node, CXA_DESC_HEADER_BYTE),
     dq_fields[1], build_int_cst (unsigned_char_type_node, 5),
     dq_fields[2], build_int_cst (unsigned_char_type_node, CXA_FIELD_SOURCE_LOCATION),
     dq_fields[3], build_int_cst (unsigned_char_type_node, CXA_FIELD_COMMENT),
     dq_fields[4], build_int_cst (unsigned_char_type_node, CXA_FIELD_MESSAGE),
     dq_fields[5], build_int_cst (unsigned_char_type_node, CXA_FIELD_QUERY_FUNCTION),
     dq_fields[6], build_int_cst (unsigned_char_type_node, CXA_FIELD_LABEL_PTR),
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
     df_fields[0], build_int_cst (unsigned_char_type_node, CXA_DESC_HEADER_BYTE),
     df_fields[1], build_int_cst (unsigned_char_type_node, 6),
     df_fields[2], build_int_cst (unsigned_char_type_node, CXA_FIELD_SOURCE_LOCATION),
     df_fields[3], build_int_cst (unsigned_char_type_node, CXA_FIELD_COMMENT),
     df_fields[4], build_int_cst (unsigned_char_type_node, CXA_FIELD_MESSAGE),
     df_fields[5], build_int_cst (unsigned_char_type_node, CXA_FIELD_LOCAL_HANDLER),
     df_fields[6], build_int_cst (unsigned_char_type_node, CXA_FIELD_QUERY_FUNCTION),
     df_fields[7], build_int_cst (unsigned_char_type_node, CXA_FIELD_LABEL_PTR),
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
      return void_node;
    case CES_ENFORCE:
    case CES_OBSERVE:
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
