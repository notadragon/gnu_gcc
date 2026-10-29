/* P4405 proof shadows: the middle-end half of -fcontracts-prove-shadow.

   Copyright (C) 2026 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* The front ends (through c-family/contracts-prove-shadow.cc) defer each
   prove or diagnose evaluation that no front-end strategy proved: they
   register it here as an obligation and place a region in the function
   body (see CONTRACT_SHADOW_GUARD in internal-fn.def).  contracts-shadow.cc
   copies each function holding an obligation into a proof shadow, which the
   ordinary pipeline optimizes and which is diagnosed and discarded, never
   emitted; it records a result per obligation and issues the diagnostics at
   the end of the compilation.  */

#ifndef GCC_CONTRACTS_SHADOW_H
#define GCC_CONTRACTS_SHADOW_H

/* What the shadows established about one obligation.  The values follow
   c-family's contract_proof_result, which this file cannot use (lto1 links
   it without the C family).  */
enum contract_shadow_result
{
  /* Not (yet) decided by any shadow.  At the end of the compilation it is
     diagnosed as unproven.  */
  CSR_PENDING,
  CSR_UNPROVEN,
  /* Unproven, and the predicate is false whenever the evaluation happens.  */
  CSR_ALWAYS_FALSE,
  /* Proven, relying on an assumption that is not guaranteed.  */
  CSR_CONDITIONAL,
  CSR_SOUND
};

/* Register an obligation: the evaluation at LOC, in the body of FNDECL, of
   a contract assertion whose predicate is at PRED_LOC (UNKNOWN_LOCATION for
   none), with the prove semantic if PROVE and with diagnose otherwise.
   SIDE_EFFECTS if the predicate may have side effects other than through
   calls to the functions in CALLEES, which it calls directly; whether
   those have any is decided once the shadow is optimized (a function not
   declared pure or const may be found to be).  CONTEXT is the front end's
   handle on the context in which it described the evaluation, 0 for none
   (contract_shadow_diagnostic_context).  PAIR is 0 for an ordinary
   obligation, decided in FNDECL's proof shadow, and otherwise the override
   pair (contract_shadow_register_override) whose override shadow decides
   it.  NAMED is 0, or the override pair for which the evaluation is made,
   whose overridden function the diagnostic names: PAIR, unless the
   evaluation is made in the ordinary shadow (-fno-contracts-prove-override),
   where PAIR is 0.  An obligation with a NAMED pair is diagnosed only if
   FNDECL is compiled.  Returns the id the front end passes to
   .CONTRACT_UNPROVEN and .CONTRACT_PROVEN.  */
extern unsigned contract_shadow_register_obligation (tree fndecl,
						     location_t loc,
						     location_t pred_loc,
						     bool prove,
						     bool side_effects,
						     const vec<tree> &callees,
						     unsigned context,
						     unsigned named = 0,
						     unsigned pair = 0);

/* Register an override pair (D4405R0, "Virtual Functions"): the virtual
   function FNDECL, which is defined here, and BASE_FN, a function it
   overrides.  An override shadow of FNDECL takes BASE_FN's preconditions as
   given and decides the obligations registered with the pair.  Returns the
   pair's number, never 0, which is also the kind of the regions the front
   end places for it (CONTRACT_SHADOW_GUARD in internal-fn.def).  */
extern unsigned contract_shadow_register_override (tree fndecl,
						   tree base_fn);

/* Note that a front end placed a region of either kind.  */
extern void contract_shadow_note_region ();

/* Note that a front end placed an assumption of an obligation's predicate
   guarded by .CONTRACT_PROVEN (-fcontracts-prove-assume).  */
extern void contract_shadow_note_proven_guard ();

/* Register an evaluation in the body of FNDECL that a front-end strategy
   decided not to be proven (ALWAYS_FALSE if its predicate is always false),
   to be diagnosed with the obligations, in order, at the end of the
   compilation: an error now would stop the compilation before the shadows
   are optimized.  The other arguments are as for
   contract_shadow_register_obligation.  The diagnostic of an evaluation of
   an override pair is issued only if FNDECL is compiled, as for the pair's
   obligations.  */
extern void contract_shadow_defer_diagnostic (tree fndecl, location_t loc,
					      location_t pred_loc,
					      bool prove, bool always_false,
					      unsigned context,
					      unsigned pair = 0);

/* Set by the front end that registers obligations with a nonzero context:
   called with that CONTEXT around each such obligation's diagnostic at the
   end of the compilation, ENTER true before it and false after it, to
   re-establish the context the evaluation was described in (in C++ the
   template instantiation backtrace).  */
extern void (*contract_shadow_diagnostic_context) (unsigned context,
						   bool enter);

/* True if an obligation is registered and not decided yet.  */
extern bool contract_shadow_pending_p ();

/* The result for obligation ID so far; final once every shadow has been
   optimized, which happens before any other function's late optimizations
   (contract_shadow_expand_first), or under -flto before anything is
   streamed.  */
extern contract_shadow_result contract_shadow_obligation_result (unsigned id);

/* Expand every proof shadow still to be processed, before anything else is
   expanded (cgraphunit.cc), so that every obligation is decided when the
   functions holding its .CONTRACT_PROVEN guards are optimized.  Returns how
   many were expanded.  */
extern int contract_shadow_expand_first ();

/* True if FNDECL is a proof shadow.  */
extern bool contract_shadow_function_p (const_tree fndecl);

/* True if NODE is a proof shadow, or inlined into one, and the inliner has
   run: what it calls and refers to is then only kept as a declaration
   unless something else needs it (ipa.cc).  */
extern bool contract_shadow_boundary_p (cgraph_node *node);

/* Called whenever the current function becomes FNDECL (function.cc):
   warnings are inhibited while a shadow is current, since its real function
   issues them.  */
extern void contract_shadow_switch_function (tree fndecl);

/* Diagnose every obligation not proven: called once the compilation unit
   has been compiled, or has failed to be (cgraphunit.cc).  */
extern void contract_shadow_finish ();

/* Premises decided by this translation unit (contracts-shadow.cc).  */

/* Note that the body of FNDECL holds a contract evaluation whose code lets
   the optimizers take its predicate to hold -- a terminating check, an
   assumption -- and that another translation unit's evaluation of the same
   assertion need not (c-family's contract_proof_note_premise).  */
extern void contract_shadow_note_premise (tree fndecl);

/* Called by the inliner as it inlines FN at the call STMT into the body of
   DST (tree-inline.cc): DST's body then holds what FN's holds, and a proof
   shadow records what its proofs may rely on this TU's code for.  */
extern void contract_shadow_note_inline (cgraph_node *dst, tree fn,
					 gimple *stmt);

#endif /* GCC_CONTRACTS_SHADOW_H */
