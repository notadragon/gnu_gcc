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
   FNDECL is compiled.  PARENT is 0, or one more than the id of the
   obligation it is nested in (DECISIONS.md E169: the analysis evaluation
   of a precondition of a call in that one's predicate, whose marker is in
   that one's region): a nested obligation with the prove semantic that is
   not proven makes every obligation it is nested in unproven, and one
   proven only conditionally makes them conditional; it is not diagnosed
   on its own, but noted with their diagnostics.  A nested obligation with
   diagnose is diagnosed on its own and counts for nothing else.
   ALWAYS_FALSE is true if a front-end strategy found the predicate false
   wherever it is evaluated (DECISIONS.md E195): the obligation is then
   "always false" if not proven, and proven only if never reached.
   Returns the id the front end passes to .CONTRACT_UNPROVEN and
   .CONTRACT_PROVEN.  */
extern unsigned contract_shadow_register_obligation (tree fndecl,
						     location_t loc,
						     location_t pred_loc,
						     bool prove,
						     bool side_effects,
						     const vec<tree> &callees,
						     unsigned context,
						     unsigned named = 0,
						     unsigned pair = 0,
						     unsigned parent = 0,
						     bool always_false = false);

/* The kind of the guard of a nested obligation's region (see
   contract_shadow_register_obligation's PARENT): `if
   (.CONTRACT_SHADOW_GUARD (CONTRACT_SHADOW_NESTED_KIND)) { if (!pred)
   .CONTRACT_UNPROVEN (id); __builtin_trap (); }', placed ahead of the call
   in the region of the obligation it is nested in.  The guard is false in
   the real function and left unknown in a shadow, so the region is a side
   branch of the path it is on that never joins it again: the shadow
   decides the obligation where the call is evaluated, but takes nothing
   from it for the code that follows -- the call, the rest of the
   predicate, and other evaluations of the same precondition, at the call
   in a check of the predicate (DECISIONS.md E169 (5)).  */
#define CONTRACT_SHADOW_NESTED_KIND (~0U)

/* Register an evaluation in the body of FNDECL nested in the obligation
   PARENT (one more than its id, as for
   contract_shadow_register_obligation) that a front-end strategy decided
   now, with the result R, so that R counts for PARENT; the other arguments
   are as for contract_shadow_register_obligation (NAMED, the override pair
   it is made for, only names it in its diagnostic: whether it is
   diagnosed follows PARENT).  Returns its id.  */
extern unsigned contract_shadow_record_nested (tree fndecl, location_t loc,
					       location_t pred_loc, bool prove,
					       contract_shadow_result r,
					       unsigned context,
					       unsigned parent,
					       unsigned named = 0);

/* A note on a diagnostic: a nested evaluation (E169) that made the one
   diagnosed unproven, at its predicate LOC, ALWAYS_FALSE if it is always
   false.  */
struct contract_shadow_note
{
  location_t loc;
  bool always_false;
};

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

/* Register CALLEES, not empty, the functions the predicate of an
   assumption region calls directly (the predicate has no other side
   effects): a proof relying on the region is conditional unless each of
   them is found const or pure, terminating and not throwing when the
   shadow is decided, as for an obligation's.  Returns the number the
   front end passes as the third argument of .CONTRACT_ASSUMPTION, never
   0, which stands for no callees.  */
extern unsigned contract_shadow_register_assumption (const vec<tree> &callees);

/* Note that a front end placed an ordinary assumption region of an
   evaluation that is not guaranteed (observe, noexcept_observe or ignore),
   which a premise copy uses (-fcontracts-prove-callee-premises): the
   functions holding such regions that a shadow reaches then get premise
   copies (contracts-shadow.cc).  */
extern void contract_shadow_note_premise_region ();

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
   obligations.  NOTES are noted with it.  */
extern void contract_shadow_defer_diagnostic
  (tree fndecl, location_t loc, location_t pred_loc, bool prove,
   bool always_false, unsigned context, unsigned pair = 0,
   const vec<contract_shadow_note> &notes = vNULL);

/* Set by the front end that registers obligations with a nonzero context:
   called with that CONTEXT around each such obligation's diagnostic at the
   end of the compilation, ENTER true before it and false after it, to
   re-establish the context the evaluation was described in (in C++ the
   template instantiation backtrace).  */
extern void (*contract_shadow_diagnostic_context) (unsigned context,
						   bool enter);

/* D4405's proof optimization level.  Set by the front end that places
   regions: the optimization options of the analysis-only functions (proof
   shadows and the copies they call) at the proof level, the command line's
   with only the optimization level replaced.  Unset, the analysis-only
   functions keep their functions' own options.  */
extern tree (*contract_shadow_proof_optimization) ();

/* True if this translation unit may make proof shadows at all: it is
   optimized (-O1 and up), or the front end gives the analysis-only
   functions a proof level (contract_shadow_proof_optimization), whatever
   the optimization level of the code, -O0 included.  Otherwise, at -O0,
   folding is final.  A function's own optimize attribute does not matter:
   the shadows are optimized at the proof level whatever it is.  */
extern bool contract_shadow_level_p ();

/* True while the bodies of external functions (DECL_EXTERNAL definitions:
   inline members of a class template instantiation declared by `extern
   template', gnu_inline functions) are to be analyzed and kept until the
   proof shadows have been made, although the translation unit is not
   optimized: shadows may be made, and may call them (cgraphunit.cc,
   ipa.cc).  When optimizing they are analyzed and kept anyway; they are
   never emitted.  */
extern bool contract_shadow_external_bodies_p ();

/* True if analysis-only code optimized at the proof level may exist,
   although the translation unit is not optimized: the IPA passes whose
   global gate is off at -O0 then run for the functions optimized at that
   level (ipa-pure-const.cc), each of which they already decide by its own
   options.  */
extern bool contract_analysis_ipa_p ();

/* True if an obligation is registered and not decided yet.  */
extern bool contract_shadow_pending_p ();

/* DECISIONS.md E169 (9): the obligations and deferred diagnostics
   registered from now on are made for GROUP -- one evaluation of a
   contract assertion, its analysis evaluation followed by its runtime
   check -- or, if GROUP is 0, for none; for the check if CHECK.  A nested
   evaluation with diagnose made for the analysis evaluation and an
   evaluation made for the check, at the same call in the predicate, with
   the same diagnostic, are diagnosed once.  */
extern void contract_shadow_set_report_once (unsigned group, bool check);

/* The result for obligation ID so far, with the obligations nested in it
   (contract_shadow_register_obligation); final once every shadow has been
   optimized, which happens before any other function's late optimizations
   (contract_shadow_expand_first), or under -flto before anything is
   streamed.  */
extern contract_shadow_result contract_shadow_obligation_result (unsigned id);

/* Expand every proof shadow still to be processed, before anything else is
   expanded (cgraphunit.cc), so that every obligation is decided when the
   functions holding its .CONTRACT_PROVEN guards are optimized.  Returns how
   many were expanded.  */
extern int contract_shadow_expand_first ();

/* True if FNDECL is a proof shadow (a premise shadow included).  */
extern bool contract_shadow_function_p (const_tree fndecl);

/* True if NODE is a proof shadow, or inlined into one, and the inliner has
   run: what it calls and refers to is then only kept as a declaration
   unless something else needs it (ipa.cc).  */
extern bool contract_shadow_boundary_p (cgraph_node *node);

/* While HOLD, contract_shadow_boundary_p holds for no node: the inliner's
   first removal after inlining keeps what only the shadows call, which it
   may still inline into them when it inlines functions into all their
   callers.  Returns true if there are shadows, when a removal must follow
   that decision.  */
extern bool contract_shadow_boundary_hold (bool hold);

/* GCC-907: true if NODE is needed only by analysis-only code -- a proof
   shadow, a body inlined into one, or a symbol nothing but such code calls
   or refers to.  The real code is compiled as if analysis-only code did not
   exist: IPA's decisions about it do not count such callers, sizes or
   references, while the analysis-only code itself is optimized as usual.  */
extern bool contract_analysis_only_p (symtab_node *node);

/* GCC-1000: true if the function being compiled is real code and NODE's
   definition is there only for analysis-only code: nothing real needs it,
   and it is not a virtual function the real code's polymorphic calls keep
   under -fdevirtualize.  The real code then may not come to refer to it --
   a load from an external vtable is not folded to it
   (can_refer_decl_in_current_unit_p) -- since without the analysis-only
   code it would not be defined here.  The same holds of a definition only
   a region removed from the real code needed, with or without a shadow.  */
extern bool contract_analysis_only_definition_p (symtab_node *node);
/* True if E is a call from analysis-only code to code that is not: IPA's
   decisions about the callee do not count it.  (A callee that is itself
   analysis-only is judged by its calls as usual.)  */
extern bool contract_analysis_only_call_p (cgraph_edge *e);

/* GCC-997: true if a body inlined into DST, the function being compiled
   or the root of an inline tree, is inlined into analysis-only code: the
   inlined function is then not described as inlined in the debug
   information, nor marked possibly inlined.  */
extern bool contract_analysis_only_inline_p (cgraph_node *dst);

/* GCC-997: what each block of the current function, whose CFG has just
   been built, is to assign_discriminators (tree-cfg.cc), so that the real
   code is given the discriminators it has without the regions, which are
   gone from it before it is compiled.  */
enum contract_discrim_kind
{
  /* Real code.  */
  CONTRACT_DISCRIM_REAL,
  /* A block only a region executes: not counted at all.  */
  CONTRACT_DISCRIM_REGION,
  /* A block ending in a region's guard, which is not counted.  */
  CONTRACT_DISCRIM_GUARD,
  /* Likewise, and the next block counted goes on from it, as one block
     would without the region.  */
  CONTRACT_DISCRIM_GUARD_JOINED
};

/* Set KINDS, indexed by block index, to the contract_discrim_kind of each
   block of the current function, and return true; or return false if the
   function holds no region.  */
extern bool contract_shadow_discriminator_kinds (vec<unsigned char> &kinds);

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
