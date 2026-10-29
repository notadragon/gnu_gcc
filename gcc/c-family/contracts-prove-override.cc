/* The override shadows' proving strategy for the P4405 prove and diagnose
   evaluation semantics (-fcontracts-prove-override).  See
   contracts-prove.h.

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

/* D4405R0, "Virtual Functions": for each function B::f that a virtual
   function D::f overrides, an override shadow of D::f's body takes B::f's
   preconditions as given and proves D::f's preconditions, D::f's
   postconditions and B::f's postconditions.  The front end places the
   regions of each override pair in D::f's body; this strategy defers the
   evaluations made in an override shadow, with the proof shadows' markers
   (contracts-prove-shadow.cc), and the middle end (contracts-shadow.cc)
   makes one shadow per pair and decides them.  An override shadow costs
   an optimization of the body per overridden function, which is why it is
   a strategy of its own.

   Without it, an override pair's evaluations are obligations of D::f
   itself, with no premise from B::f: D::f's preconditions are settled
   only if they fold, and B::f's postconditions are proven in D::f's
   ordinary shadow, if any (contract_proof_query's override_shadow).  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tree.h"
#include "options.h"
#include "c-family/contracts-prove.h"

static bool
override_enabled_p ()
{
  return flag_contracts_prove_override;
}

static bool
override_defers_p (tree fndecl)
{
  return (flag_contracts_prove_override
	  && contract_prove_shadow_may_defer_p (fndecl));
}

static contract_proof_result
override_prove (const contract_proof_query &query)
{
  if (!query.override_pair
      || !query.override_shadow
      || !query.deferrable
      || !override_defers_p (current_function_decl))
    return CPR_UNPROVEN;
  return CPR_DEFERRED;
}

/* The marker of QUERY, an override shadow's evaluation: the proof shadows'
   region, of the pair's kind.  */

static tree
override_mark (const contract_proof_query &query, tree pred)
{
  return contract_prove_shadow_strategy.mark (query, pred);
}

/* The region letting override pair PAIR's shadow assume PRED, the
   predicate of an evaluation at LOC with semantic SEM: a premise from the
   overridden function's preconditions.  The ordinary shadow's (PAIR 0)
   are the proof shadows' strategy's.  */

static tree
override_assume (tree pred, contract_evaluation_semantic sem, location_t loc,
		 unsigned pair)
{
  if (!pair)
    return NULL_TREE;
  return contract_prove_shadow_assumption (pred, sem, loc, pair);
}

/* The diagnostics wait for the override shadows' as for the proof
   shadows' (shadow_take_diagnostic).  */

static bool
override_take_diagnostic (const contract_proof_query &query,
			  bool always_false)
{
  return contract_prove_shadow_strategy.take_diagnostic (query, always_false);
}

/* What an override shadow may take as a premise is noted as for the proof
   shadows (shadow_note_premise).  */

static void
override_note_premise (tree fndecl)
{
  contract_prove_shadow_strategy.note_premise (fndecl);
}

bool
contract_proof_override_shadows_p (tree fndecl)
{
  return override_defers_p (fndecl);
}

const contract_proof_strategy contract_prove_override_strategy = {
  "override",
  /*may_depend_on_assumptions=*/true,
  override_enabled_p,
  override_prove,
  override_defers_p,
  override_mark,
  override_assume,
  override_take_diagnostic,
  override_note_premise
};
