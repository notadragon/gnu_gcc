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

/* The registry: every strategy, in the order they are asked, cheaper ones
   first, then a null pointer.  */

static const contract_proof_strategy *const contract_proof_strategies[] = {
  &contract_prove_fold_strategy,
  NULL
};

contract_proof_result
contract_prove (const contract_proof_query &query,
		const contract_proof_strategy **deferrer)
{
  gcc_checking_assert (contract_semantic_proving_p (query.semantic));
  gcc_checking_assert (!query.opaque || !query.deferrable);
  if (query.opaque)
    return CPR_UNPROVEN;
  contract_proof_result best = CPR_UNPROVEN;
  const contract_proof_strategy *deferring = NULL;
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
      if (r == CPR_SOUND || r == CPR_ALWAYS_FALSE)
	return r;
      if (r == CPR_CONDITIONAL)
	best = r;
      if (r == CPR_DEFERRED && !deferring)
	deferring = s;
    }
  if (best == CPR_UNPROVEN && deferring)
    {
      if (deferrer)
	*deferrer = deferring;
      return CPR_DEFERRED;
    }
  return best;
}

contract_proof_result
contract_diagnose_unless_proven (const contract_proof_query &query,
				 tree *marker, tree pred)
{
  gcc_checking_assert (contract_semantic_proving_p (query.semantic));
  gcc_checking_assert (!query.deferrable || marker);
  if (flag_syntax_only)
    return CPR_UNPROVEN;

  const contract_proof_strategy *deferrer = NULL;
  contract_proof_result r = contract_prove (query, &deferrer);
  if (r == CPR_DEFERRED)
    {
      *marker = deferrer->mark (query, pred ? pred : query.predicate);
      return r;
    }
  if (contract_proof_proven_p (r))
    return r;

  bool always_false = r == CPR_ALWAYS_FALSE;
  /* An override pair's diagnostic waits for its pair's even when the
     strategy keeping the pairs is not proving.  */
  for (const contract_proof_strategy *const *p = contract_proof_strategies;
       *p; ++p)
    if ((*p)->take_diagnostic
	&& ((*p)->enabled_p () || query.override_pair)
	&& (*p)->take_diagnostic (query, always_false))
      return r;

  auto_diagnostic_group d;
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

