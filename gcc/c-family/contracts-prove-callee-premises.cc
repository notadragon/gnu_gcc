/* Proving strategies for the P4405 prove and diagnose evaluation semantics:
   callee premise copies (-fcontracts-prove-callee-premises).  See
   contracts-prove.h, and contracts-shadow.cc for the middle end.

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

/* D4405R0 Section 4.5: an evaluation with observe, noexcept_observe or
   ignore is a premise that makes a later proof conditional, and a
   conditional proof is not diagnosed -- also when the evaluation is in a
   callee that the proof shadow inlines.  A proof shadow takes such a
   premise from the assumption region placed after the evaluation
   (contracts-prove-shadow.cc), but the functions it calls are the real
   ones, whose regions are removed before anything is inlined, so a
   callee's postcondition or contract_assert says nothing after the call.

   With this strategy, each function holding such a region that a shadow
   may reach gets a premise copy, whose regions assume their predicates as
   in a shadow and which holds no obligation, and a shadow that calls such
   a function, directly or through the functions it calls, gets a second
   shadow that calls the copies instead.  The middle end decides the
   ordinary shadows exactly as without the strategy, and an obligation one
   leaves unproven that the second shadow proves is proven conditionally:
   not diagnosed, never assumed (contracts-shadow.cc).  Nothing is decided
   here, and the strategy places no region of its own: it only tells the
   middle end, from its assumption hook, which runs ahead of the proof
   shadows' (whose region it leaves in place), that a function holds a
   region a copy can use.  The copies cost an optimization of each such
   function and of each shadow that reaches one, which is why this is a
   strategy of its own.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tree.h"
#include "options.h"
#include "c-family/contracts-prove.h"
#include "contracts-shadow.h"

static bool
callee_premises_enabled_p ()
{
  return flag_contracts_prove_callee_premises;
}

/* Nothing is proven here: the middle end decides with the shadows.  */

static contract_proof_result
callee_premises_prove (const contract_proof_query &)
{
  return CPR_UNPROVEN;
}

/* PRED, the predicate of an evaluation with semantic SEM, is to get the
   proof shadows' assumption region (contract_prove_shadow_assumption) when
   it is an ordinary one (PAIR 0): if that region is one a premise copy
   uses -- an evaluation that is not guaranteed -- switch the copies on.
   No region of its own.  */

static tree
callee_premises_assume (tree pred, contract_evaluation_semantic sem,
			location_t, unsigned pair)
{
  auto_vec<tree> callees;
  if (pair
      || (sem != CES_IGNORE && sem != CES_OBSERVE
	  && sem != CES_NOEXCEPT_OBSERVE)
      || !flag_contracts_prove_shadow
      || !contract_prove_shadow_may_defer_p (current_function_decl)
      || !contract_prove_shadow_assumable_p (pred, callees))
    return NULL_TREE;
  contract_shadow_note_premise_region ();
  return NULL_TREE;
}

const contract_proof_strategy contract_prove_callee_premises_strategy = {
  "callee-premises",
  /*may_depend_on_assumptions=*/true,
  callee_premises_enabled_p,
  callee_premises_prove,
  /*defers_p=*/NULL,
  /*mark=*/NULL,
  callee_premises_assume,
  /*take_diagnostic=*/NULL,
  /*note_premise=*/NULL,
  /*nest_decided=*/NULL,
  /*nested_predicate=*/NULL
};
