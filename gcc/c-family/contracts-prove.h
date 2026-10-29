/* Proving strategies for the P4405 prove and diagnose evaluation semantics.

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

/* An evaluation of a contract assertion with the P4405 prove or diagnose
   semantic is diagnosed -- an error for prove, a -Wcontract-unproven warning
   for diagnose -- unless its predicate can be proven to hold.  The front
   ends describe each such evaluation in a contract_proof_query, at the point
   where its check would have been emitted, and make the one call
   contract_diagnose_unless_proven.  It asks each enabled proving strategy in
   turn and diagnoses the evaluation when none proves it.

   Each strategy lives in a file of its own (contracts-prove-<name>.cc),
   behind contract_proof_strategy, is switched by its own
   -fcontracts-prove-<name> flag (implied by -fcontracts-p4405), and is
   listed once in contracts-prove.cc's registry.  With every strategy off,
   every evaluation is diagnosed.

   The files are compiled into both cc1 (C) and cc1plus (C++) via
   C_COMMON_OBJS; a strategy that needs the front end, as folding does,
   declares a hook here that each front end defines.  */

#ifndef GCC_C_FAMILY_CONTRACTS_PROVE_H
#define GCC_C_FAMILY_CONTRACTS_PROVE_H

#include "c-family/contracts-config.h"

/* What a strategy established about one evaluation.  */
enum contract_proof_result
{
  /* Not proven.  */
  CPR_UNPROVEN,
  /* Not proven, and moreover the predicate is false whenever the evaluation
     happens: a refinement of CPR_UNPROVEN, diagnosed as "always false".  A
     strategy returns it only when that is as certain as a sound proof.  */
  CPR_ALWAYS_FALSE,
  /* Proven, but relying on an earlier assertion that is not guaranteed to
     hold (one evaluated with ignore, observe or noexcept_observe, or itself
     unproven or only conditionally proven).  Not diagnosed.  */
  CPR_CONDITIONAL,
  /* Proven without relying on anything not guaranteed.  Not diagnosed.  */
  CPR_SOUND,
  /* Not decided yet: a strategy that proves later in the compilation took
     the evaluation over (only when the query is deferrable).  Not diagnosed
     now; the front end places the strategy's marker where the evaluation
     happens (contract_diagnose_unless_proven), and the strategy diagnoses
     the evaluation if it is not proven.  */
  CPR_DEFERRED
};

/* True if R suppresses the diagnostic.  */

inline bool
contract_proof_proven_p (contract_proof_result r)
{
  return r == CPR_SOUND || r == CPR_CONDITIONAL;
}

/* One evaluation to prove.  */

struct contract_proof_query
{
  /* The predicate, after any template instantiation and converted to a
     truth value, as the check would have evaluated it.  Strategies never
     evaluate it at run time and never modify it.  */
  tree predicate;
  /* CES_PROVE or CES_DIAGNOSE.  */
  contract_evaluation_semantic semantic;
  contract_assertion_kind kind;
  /* Where the evaluation happens: the assertion, for a callee-side
     evaluation; the call, for a caller-side one.  */
  location_t loc;
  /* The predicate's own location, noted with the diagnostic when it differs
     from LOC; UNKNOWN_LOCATION for none.  */
  location_t pred_loc;
  /* For a caller-side evaluation of a precondition, the CALL_EXPR, and the
     chain of the callee's PARM_DECLs that the predicate names, in the order
     of the call's arguments (an artificial parameter other than `this' is
     never in it).  Otherwise both NULL_TREE.  */
  tree call;
  tree parms;
  /* True if the front end can place a marker where the evaluation happens,
     so that a strategy may defer it (CPR_DEFERRED).  */
  bool deferrable;
  /* True if the front end cannot describe the predicate as the check would
     evaluate it (a template's, which it could not substitute): no strategy
     is asked, and the evaluation is not proven.  PREDICATE is then not to
     be looked at.  */
  bool opaque;
  /* 0 for an ordinary evaluation.  Otherwise the override pair for which
     the evaluation is made, in the body of the overriding function, on the
     assumption that the overridden function's preconditions hold (D4405R0,
     "Virtual Functions"; contract_shadow_register_override): its
     diagnostic is taken over by a strategy (take_diagnostic) even when
     that strategy is not enabled, so that it can be issued with the pair's
     at the end of the compilation, naming the overridden function, and
     only if the overriding function is compiled.  */
  unsigned override_pair;
  /* For an override pair's evaluation, true if it is made in the pair's
     override shadow (-fcontracts-prove-override), on the assumption that
     the overridden function's preconditions hold; false if it is made in
     the overriding function's ordinary proof shadow, with no such
     assumption, as an obligation of the overriding function itself.  */
  bool override_shadow;
};

/* A proving strategy.  */

struct contract_proof_strategy
{
  /* Its name, as in -fcontracts-prove-<name>.  */
  const char *name;
  /* True if its proofs may rely on earlier assertions -- assume that they
     hold -- and so may be CPR_CONDITIONAL.  False if every proof it makes
     is sound.  */
  bool may_depend_on_assumptions;
  /* True if it is enabled (its -fcontracts-prove-<name> flag).  */
  bool (*enabled_p) ();
  /* Try to prove QUERY.  */
  contract_proof_result (*prove) (const contract_proof_query &query);

  /* The rest are for a strategy that proves later in the compilation, and
     are null for one that answers at once.  */

  /* True if it may defer evaluations in the body of FNDECL.  */
  bool (*defers_p) (tree fndecl);
  /* The marker the front end places where QUERY, which it deferred, is
     evaluated; PRED is the predicate as a check there would evaluate it.  */
  tree (*mark) (const contract_proof_query &query, tree pred);
  /* The statement letting it assume PRED, the predicate of an evaluation at
     LOC with semantic SEM, which the front end places after the evaluation's
     check; NULL_TREE if it wants none.  PAIR is 0, or the override pair for
     whose evaluations alone the assumption is made (contract_proof_query's
     override_pair).  */
  tree (*assume) (tree pred, contract_evaluation_semantic sem, location_t loc,
		  unsigned pair);
  /* If the diagnostic of QUERY, decided now not to be proven (ALWAYS_FALSE if
     always false), must wait for its own -- to keep the diagnostics in
     order, and an error from stopping the compilation before it has proven
     anything -- take it over and return true.  */
  bool (*take_diagnostic) (const contract_proof_query &query,
			   bool always_false);
  /* Told that the body of FNDECL holds an evaluation whose code lets the
     optimizers take its predicate to hold where the function continues
     (a terminating check or an assumption), and that another translation
     unit's evaluation of the same assertion need not
     (contract_proof_note_premise).  */
  void (*note_premise) (tree fndecl);
};

/* Ask each enabled strategy about QUERY, in registry order: the first sound
   proof or "always false" answer wins; otherwise a conditional proof, if
   any; otherwise deferred, if a strategy deferred it (the first, stored in
   *DEFERRER if non-null); otherwise unproven.  */
extern contract_proof_result
contract_prove (const contract_proof_query &,
		const contract_proof_strategy **deferrer = NULL);

/* The front ends' one entry point.  Prove QUERY, and when it is not proven
   diagnose the evaluation: "contract assertion cannot be proven", or
   "contract predicate is always false", an error for prove and a
   -Wcontract-unproven warning for diagnose.  Nothing is done under
   -fsyntax-only, where no check would be emitted.  Returns the result.  For
   a deferrable QUERY, MARKER is non-null, and for CPR_DEFERRED it is set to
   the marker to place where the evaluation happens, for the predicate PRED
   (QUERY's own when null).  */
extern contract_proof_result
contract_diagnose_unless_proven (const contract_proof_query &,
				 tree *marker = NULL, tree pred = NULL_TREE);

/* True if an enabled strategy may defer evaluations in FNDECL's body: the
   front end then offers markers and places assumptions there.  */
extern bool contract_proof_defers_p (tree fndecl);

/* The statement letting the strategies that defer evaluations assume PRED,
   the predicate of an evaluation at LOC with semantic SEM, for the front end
   to place after the evaluation's check; NULL_TREE if none wants one.  If
   PAIR is not 0, the assumption is made only for the evaluations of that
   override pair (contract_proof_query's override_pair).  */
extern tree contract_proof_assumption (tree pred,
				       contract_evaluation_semantic sem,
				       location_t loc, unsigned pair = 0);

/* Tell the strategies that defer evaluations that the body of FNDECL holds
   an evaluation whose code lets the optimizers take its predicate to hold
   where the function continues -- a check with a terminating semantic, or
   an assumption -- and that another translation unit's evaluation of the
   same assertion need not: its allowed set admits a semantic that continues
   past a violation, or it is a caller-side evaluation.  A front end calls
   it where it emits such an evaluation.  What follows from that code holds
   only where this translation unit's code for FNDECL is what runs.  */
extern void contract_proof_note_premise (tree fndecl);

/* The strategies, each defined in its own contracts-prove-<name>.cc.  */

/* -fcontracts-prove-fold.  */
extern const contract_proof_strategy contract_prove_fold_strategy;

/* -fcontracts-prove-shadow (contracts-prove-shadow.cc; the middle end's
   contracts-shadow.cc does the proving).  */
extern const contract_proof_strategy contract_prove_shadow_strategy;

/* For the proof shadows of either kind: true if FNDECL's evaluations may
   go to a proof shadow, whichever strategy's flag is set.  */
extern bool contract_prove_shadow_may_defer_p (tree fndecl);

/* For the proof shadows of either kind: the region of kind KIND letting a
   shadow assume PRED, the predicate of an evaluation at LOC with semantic
   SEM, or NULL_TREE (contract_proof_strategy's assume).  */
extern tree contract_prove_shadow_assumption (tree pred,
					      contract_evaluation_semantic sem,
					      location_t loc, unsigned kind);

/* Defined by each front end, for the folding strategy: EXPR folded as far
   as the front end can fold it -- in C++ including constant evaluation that
   is not manifestly constant-evaluated -- without changing EXPR or anything
   else about the program.  */
extern tree c_contract_fold_predicate (tree expr);

/* Defined by each front end, for a strategy that diagnoses an evaluation
   after the front end described it (the proof shadows): a handle on the
   context the front end is describing an evaluation in now -- in C++ the
   template instantiation backtrace, "In instantiation of ..." and
   "required from here" -- or 0 if it has none (always, in C).  */
extern unsigned c_contract_diagnostic_context ();

/* Re-establish CONTEXT, a nonzero handle from c_contract_diagnostic_context,
   for a diagnostic when ENTER, and what it replaced when not.  */
extern void c_contract_switch_diagnostic_context (unsigned context,
						  bool enter);

#endif /* GCC_C_FAMILY_CONTRACTS_PROVE_H */
