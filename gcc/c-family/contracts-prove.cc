/* Proving strategies for the P4405 prove and diagnose evaluation semantics:
   the registry and the dispatcher.  See contracts-prove.h.

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

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tree.h"
#include "options.h"
#include "diagnostic.h"
#include "c-family/contracts-prove.h"
#include "contracts-shadow.h"

/* The registry: every strategy, in the order they are asked, cheaper ones
   first, then a null pointer.  */

static const contract_proof_strategy *const contract_proof_strategies[] = {
  &contract_prove_fold_strategy,
  &contract_prove_callee_premises_strategy,
  &contract_prove_shadow_strategy,
  NULL
};

contract_proof_result
contract_prove (const contract_proof_query &query,
		const contract_proof_strategy **deferrer, bool *always_false)
{
  gcc_checking_assert (contract_semantic_proving_p (query.semantic));
  gcc_checking_assert (!query.opaque || !query.deferrable);
  if (query.opaque)
    return CPR_UNPROVEN;
  contract_proof_result best = CPR_UNPROVEN;
  const contract_proof_strategy *deferring = NULL;
  /* E195: a strategy found the predicate false wherever it is evaluated;
     one that defers may still find the evaluation never reached.  A
     nested evaluation is decided with the one it is nested in, which is
     reached only where it is.  */
  bool folded_false = false;
  for (const contract_proof_strategy *const *p = contract_proof_strategies;
       *p; ++p)
    {
      const contract_proof_strategy *s = *p;
      if (!s->enabled_p ())
	continue;
      contract_proof_result r = s->prove (query);
      gcc_checking_assert (r != CPR_CONDITIONAL
			   || s->may_depend_on_assumptions);
      gcc_checking_assert (r != CPR_DEFERRED
			   || (query.deferrable && s->mark));
      if (folded_false)
	{
	  if (r == CPR_DEFERRED)
	    {
	      if (deferrer)
		*deferrer = s;
	      if (always_false)
		*always_false = true;
	      return r;
	    }
	  continue;
	}
      if (r == CPR_ALWAYS_FALSE)
	{
	  if (!query.deferrable || query.nest || !always_false)
	    return r;
	  folded_false = true;
	  continue;
	}
      if (r == CPR_SOUND)
	{
	  /* E169: with nested evaluations to place, a strategy that defers
	     is still asked: they are only decided where it places them.  */
	  if (!query.nested)
	    return r;
	  best = r;
	}
      else if (r == CPR_CONDITIONAL && best != CPR_SOUND)
	best = r;
      if (r == CPR_DEFERRED && !deferring)
	deferring = s;
    }
  if (folded_false)
    return CPR_ALWAYS_FALSE;
  if (deferring && (best == CPR_UNPROVEN || query.nested))
    {
      if (deferrer)
	*deferrer = deferring;
      return CPR_DEFERRED;
    }
  return best;
}

/* The texts of the note for CAUSE (contract_proof_cause), shared with the
   middle end's (contracts-shadow.cc).  */

static void
inform_cause (const contract_proof_cause &cause)
{
  if (cause.always_false)
    inform (cause.loc, "precondition of a call in the predicate is always "
	    "false");
  else
    inform (cause.loc, "precondition of a call in the predicate cannot be "
	    "proven");
}

void
contract_proof_inform_causes (const vec<contract_proof_cause> &causes)
{
  for (const contract_proof_cause &c : causes)
    inform_cause (c);
}

/* QUERY's predicate as the first enabled strategy with a view of its own
   sees it (contract_proof_strategy's nested_predicate), or as it is.  */

static tree
nested_predicate (const contract_proof_query &query)
{
  for (const contract_proof_strategy *const *p = contract_proof_strategies;
       *p; ++p)
    if ((*p)->nested_predicate && (*p)->enabled_p ())
      return (*p)->nested_predicate (query);
  return query.predicate;
}

/* QUERY, decided now with the result R: make the evaluations nested in it
   (E169), which are decided now too, and return the combined result.  Its
   nested prove evaluations not proven are appended to CAUSES.  If QUERY is
   itself nested in evaluations decided now, its own result is counted for
   each of them as well.  */

static contract_proof_result
decide_nested_now (const contract_proof_query &query, contract_proof_result r,
		   vec<contract_proof_cause> *causes)
{
  bool conditional = false;
  if (query.nested)
    {
      contract_proof_nesting n = {};
      n.up = query.nest;
      n.causes = causes;
      n.conditional = &conditional;
      query.nested (query.nested_data, nested_predicate (query),
		    &n);
    }
  /* Its nested evaluations counted themselves for every evaluation above
     it as well.  */
  if (query.semantic == CES_PROVE)
    for (const contract_proof_nesting *n = query.nest; n; n = n->up)
      {
	gcc_checking_assert (n->causes && !n->strategy);
	if (!contract_proof_proven_p (r))
	  {
	    contract_proof_cause c
	      = { (query.pred_loc != UNKNOWN_LOCATION
		   ? query.pred_loc : query.loc), r == CPR_ALWAYS_FALSE };
	    n->causes->safe_push (c);
	  }
	else if (r == CPR_CONDITIONAL)
	  *n->conditional = true;
      }
  if (!causes->is_empty () && contract_proof_proven_p (r))
    return CPR_UNPROVEN;
  if (conditional && r == CPR_SOUND)
    return CPR_CONDITIONAL;
  return r;
}

contract_proof_result
contract_prove_with_nested (const contract_proof_query &query,
			    vec<contract_proof_cause> *causes)
{
  gcc_checking_assert (!query.deferrable && !query.nest);
  return decide_nested_now (query, contract_prove (query), causes);
}

/* E169 (9): what contract_proof_set_report_once set, and the diagnostics
   issued at once for the nested diagnose evaluations of the group's
   analysis evaluation, not yet matched by one of its check's.  */

struct report_once_key
{
  location_t loc;
  location_t pred_loc;
  bool always_false;
};

static unsigned report_once_group;
static bool report_once_check;
static vec<report_once_key> report_once_issued;

void
contract_proof_set_report_once (unsigned group, bool check)
{
  if (group != report_once_group)
    report_once_issued.truncate (0);
  report_once_group = group;
  report_once_check = check;
  contract_shadow_set_report_once (group, check);
}

/* QUERY, a diagnose evaluation decided now not to be proven (ALWAYS_FALSE
   if always false), is about to be diagnosed at once: true if it is made
   for a check whose analysis evaluation's nested evaluation at the same
   call gave the same diagnostic, which is then matched.  */

static bool
reported_once_p (const contract_proof_query &query, bool always_false)
{
  if (!report_once_group || !report_once_check
      || query.semantic != CES_DIAGNOSE)
    return false;
  for (unsigned i = 0; i < report_once_issued.length (); ++i)
    {
      const report_once_key &k = report_once_issued[i];
      if (k.loc == query.loc && k.pred_loc == query.pred_loc
	  && k.always_false == always_false)
	{
	  report_once_issued.ordered_remove (i);
	  return true;
	}
    }
  return false;
}

contract_proof_result
contract_diagnose_unless_proven (const contract_proof_query &query,
				 tree *marker, tree pred)
{
  gcc_checking_assert (contract_semantic_proving_p (query.semantic));
  gcc_checking_assert (!query.deferrable || marker);
  gcc_checking_assert (!query.deferrable || !query.nest
		       || query.nest->markers);
  /* A nested evaluation is made under -fsyntax-only only where the one it
     is nested in is (contract_prove_with_nested).  */
  if (flag_syntax_only && !query.nest)
    return CPR_UNPROVEN;

  const contract_proof_strategy *deferrer = NULL;
  bool folded_false = false;
  contract_proof_result r = contract_prove (query, &deferrer, &folded_false);
  if (r == CPR_DEFERRED)
    {
      *marker = deferrer->mark (query, pred ? pred : query.predicate,
				folded_false);
      return r;
    }

  /* E169: nested in an evaluation a strategy decides later, it is that
     strategy's to record, with the evaluations nested in it in turn.  */
  if (query.nest && query.nest->strategy)
    {
      unsigned handle = query.nest->strategy->nest_decided (query, r);
      if (query.nested)
	{
	  contract_proof_nesting n = {};
	  n.up = query.nest;
	  n.strategy = query.nest->strategy;
	  n.handle = handle;
	  query.nested (query.nested_data,
			nested_predicate (query), &n);
	}
      return r;
    }

  auto_vec<contract_proof_cause> causes;
  r = decide_nested_now (query, r, &causes);
  /* A nested prove evaluation counts only for those it is nested in.  */
  if (query.nest && query.semantic == CES_PROVE)
    return r;
  if (contract_proof_proven_p (r))
    return r;

  bool always_false = r == CPR_ALWAYS_FALSE;
  /* An override pair's diagnostic waits for its pair's even when the
     strategy keeping the pairs is not proving.  */
  for (const contract_proof_strategy *const *p = contract_proof_strategies;
       *p; ++p)
    if ((*p)->take_diagnostic
	&& ((*p)->enabled_p () || query.override_pair)
	&& (*p)->take_diagnostic (query, always_false, causes))
      return r;

  if (reported_once_p (query, always_false))
    return r;
  auto_diagnostic_group d;
  /* notadragon_wg21 DECISIONS.md E186 (a): a warning for an assertion whose
     predicate is not in a system header is shown even when the evaluation
     it reports is in one -- a closure's precondition analyzed where a
     library template calls it.  The assertion is the program's, which
     asked for the diagnostic and can act on it.  One whose predicate is in
     a system header stays governed by -Wsystem-headers.  Deciding by the
     predicate's location alone makes it independent of inlining.  */
  warn_system_headers_sentinel sys (query.pred_loc);
  bool emitted = true;
  if (query.semantic == CES_PROVE)
    {
      if (always_false)
	error_at (query.loc, "contract predicate is always false");
      else
	error_at (query.loc, "contract assertion cannot be proven");
    }
  else if (always_false)
    emitted = warning_at (query.loc, OPT_Wcontract_unproven,
			  "contract predicate is always false");
  else
    emitted = warning_at (query.loc, OPT_Wcontract_unproven,
			  "contract assertion cannot be proven");
  if (emitted && query.pred_loc != UNKNOWN_LOCATION
      && query.pred_loc != query.loc)
    inform (query.pred_loc, "the predicate of the contract assertion");
  if (emitted)
    contract_proof_inform_causes (causes);
  if (emitted && report_once_group && !report_once_check && query.nest
      && query.semantic == CES_DIAGNOSE)
    {
      report_once_key k = { query.loc, query.pred_loc, always_false };
      report_once_issued.safe_push (k);
    }
  return r;
}

bool
contract_proof_defers_p (tree fndecl)
{
  for (const contract_proof_strategy *const *p = contract_proof_strategies;
       *p; ++p)
    if ((*p)->defers_p && (*p)->enabled_p () && (*p)->defers_p (fndecl))
      return true;
  return false;
}

void
contract_proof_note_premise (tree fndecl)
{
  for (const contract_proof_strategy *const *p = contract_proof_strategies;
       *p; ++p)
    if ((*p)->note_premise && (*p)->enabled_p ())
      (*p)->note_premise (fndecl);
}

tree
contract_proof_assumption (tree pred, contract_evaluation_semantic sem,
			   location_t loc, unsigned pair)
{
  for (const contract_proof_strategy *const *p = contract_proof_strategies;
       *p; ++p)
    if ((*p)->assume && (*p)->enabled_p ())
      if (tree stmt = (*p)->assume (pred, sem, loc, pair))
	return stmt;
  return NULL_TREE;
}

