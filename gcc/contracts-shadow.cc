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

/* A prove or diagnose evaluation that no front-end strategy proved becomes
   an obligation (contract_shadow_register_obligation), and the front end
   places a region where the check would have been:

     if (.CONTRACT_SHADOW_GUARD ()) { if (!pred) .CONTRACT_UNPROVEN (id); }

   Earlier evaluations with ignore, observe, noexcept_observe or assume,
   whose predicate has no side effects but through the functions it calls,
   get a region too, ending in .CONTRACT_ASSUMPTION (guaranteed, kind,
   callees) -- guaranteed for assume; callees, when not 0, names the
   functions the predicate calls (contract_shadow_register_assumption).
   One with enforce, noexcept_enforce or quick_enforce needs none: its
   check calls a noreturn entry point or traps, which ensures the predicate
   on the continuing path.

   Three passes then do the work.

   1. contract_shadow_clone, a simple IPA pass before SSA is built (so that
      no warning or analysis of the real function ever sees a region): each
      function holding an obligation is copied with
      create_version_clone_with_body into its proof shadow, an internal
      function kept alive by force_output and never inlined, cloned,
      merged or split.  In the real function
      every guard is folded to false and the regions disappear: its code is
      exactly what ignore gives.  In the shadow every guard is folded to
      true; an assumption's tail becomes __builtin_unreachable (), and an
      obligation's tail stays, a noreturn marker.

   2. The shadow is optimized at the proof optimization level (D4405,
      -fcontracts-prove-optimize=, see below), inlining included.
      Whatever the optimizers can show unreachable is proven -- undefined
      behavior included, which is treated as unreachable exactly like an
      assumed predicate.  Warnings are inhibited while a shadow is the
      current function.

   3. contract_shadow, a GIMPLE pass where pass_assumptions runs, after all
      optimizations: every .CONTRACT_UNPROVEN still present is an
      unproven obligation ("always false" if it is reached from the entry
      without a condition), every other obligation of the shadow is proven,
      and the shadow is discarded, never expanded.  Under -flto the compile
      step never runs the late passes, so contract_shadow_early does this
      after the early optimizations instead, and removes the shadows before
      anything is streamed: diagnosis stays at the compile step.

   A proof is conditional rather than sound when the shadow holds an
   assumption that is not guaranteed, when it evaluates a predicate that may
   have side effects (an obligation's or an assumption's: the shadow's later
   state may then differ from the program's, and an assumption's call might
   not return), or when any of its obligations is unproven (each marker is
   noreturn, so what follows it assumes its predicate).
   Undefined behavior is a guaranteed premise: it does not make a proof
   conditional.  Both suppress the diagnostic; the distinction is recorded
   per obligation (contract_shadow_obligation_result).  A predicate's side
   effects are judged when the shadow is decided: one that calls functions
   directly has none of its own if, by then, each of them is known to be
   const or pure, terminating and not throwing -- which the IPA analyses
   often find of an inline function not declared so (std::vector::size).
   Anything else with side effects (a store, a volatile access, an indirect
   or virtual call, a temporary) counts -- and an evaluation that is not an
   obligation and whose predicate has any of those gets no region at all.

   A premise is guaranteed only where this translation unit decides it, so
   that what is linked can never turn a program without undefined behavior
   into one with it.  The assumption regions come from the function's own
   body (its callee-side evaluations and the caller-side ones at its calls)
   and are this translation unit's code.  But the code the shadow inlines
   may hold evaluations too -- a terminating check, an assumption -- and a
   function another translation unit may provide (inline, comdat, weak,
   interposable) may be linked from a copy that evaluates the same
   assertion with ignore or observe.  So the front ends note which bodies
   hold such evaluations (contract_shadow_note_premise; none for an
   assertion whose allowed set admits only terminating semantics and
   assume, which every copy makes), the inliner tells the shadow which of
   them it inlined from a replaceable function and through which chain of
   calls (contract_shadow_note_inline) -- and which refinable bodies
   (cgraph.cc), whose evaluations, even those leaving no premise here (an
   ignore evaluation has no effects; another copy's observe evaluation
   calls a violation handler, which may store to memory), another copy may
   make differently -- and a proof is conditional unless
   every copy of the real function that cgraph knows of inlined those same
   calls -- or when the shadow still calls out of line code whose IPA
   summary may rest on such evaluations.  Copies inlined early into other
   functions are checked where the assumption made of a sound proof is
   finally used: a copy that makes such a call out of line drops it
   (check_assumption_copies).  A function that keeps such an assumption
   is marked as a refinable body (mark_refinable_body), if nothing marked
   it already: where another translation unit may provide the definition
   the program calls, which assumes nothing, the callers that do not inline
   it then treat it as interposable, and use nothing its late optimizations
   learn from the assumption -- its return value's range, that it is pure,
   const, nothrow or noreturn, its memory effects.

   Under -fcontracts-prove-assume a soundly proven obligation becomes an
   assumption in the real code.  Its front end places, after the region,

     .ASSUME (!.CONTRACT_PROVEN (id) || pred);

   with a copy of the predicate.  The guard keeps the condition from being
   simple, so it is lowered as a non-simple [[assume]]'s is: outlined into
   an assumption function, never called, whose parameters' ranges
   pass_assumptions computes when that function is optimized, for the
   ranger to use at the .ASSUME call.  While the guard is unknown nothing
   follows for the predicate, so the call is inert: the real function and
   its shadow (whose copy of the call refers to the same assumption
   function) carry it through the early and IPA optimizations as an opaque
   call -- through which it may be inlined into other functions.  Every
   shadow is expanded before anything else (contract_shadow_expand_first),
   and then the assumption functions, before any real function, so the
   obligation is decided when the assumption function's late optimizations
   begin and their ranges are known when the real functions' begin.
   contract_proven then
   folds the guard: to true for a sound proof, which makes the assumption
   the predicate's, and to false otherwise (a conditional proof relies on
   something that may not hold), which makes it vacuous; a real function's
   vacuous .ASSUME calls are removed then too, so that what they took the
   address of need not stay in memory (check_assumption_copies).  Under
   -flto the guards are folded at the compile step with the shadows'
   decisions, before anything is streamed.  Should a guard reach expansion anyway it
   is false.

   An evaluation that folding finds always false is deferred to the shadow
   all the same (DECISIONS.md E195): reaching no point is a proof.  It is
   proven when its marker is removed as unreachable -- soundly unless the
   shadow is conditional -- and "always false" wherever the marker survives,
   whatever else the shadow relies on (obligation::folded_false).  Under
   -fcontracts-prove-assume a sound proof of it assumes `false' where the
   evaluation is, which says no more than the proof did: the point is
   never reached in a program without undefined behavior.

   A marker duplicated by unrolling, threading or the C++ constructor and
   destructor clones is one obligation: an obligation is unproven if any
   reachable copy is.  "Always false" needs the marker reached from the
   entry without a condition and outside any loop when the shadow was made
   (a copy of another iteration may have been proven and removed).

   Shadows keep the functions their bodies reference alive until inlining
   is done, then only as declarations (contract_shadow_boundary_p), so
   nothing is emitted for them.  Inlining is done once the inliner has
   decided which functions to inline into all their callers, which may
   inline into a shadow a function only it calls -- a level copy, or a
   function called once -- as the proof level would without the copies
   (contract_shadow_boundary_hold).

   Nor do they change how the real code is compiled when nothing is assumed
   (GCC-907).  A shadow, the bodies inlined into it and whatever only they
   call or refer to are analysis-only code (contract_analysis_only_p), and
   IPA decides about the real code as if it did not exist: a call from
   analysis-only code to the real code (contract_analysis_only_call_p) does
   not count where the inliner estimates growth and finds functions called
   once, where IPA-CP and IPA-SRA gather what the callers pass and use,
   where IPA propagates frequencies and comdat groups (the symbol table's
   checks then accept a shadow's calls of and references to a symbol made
   local to a group the shadow is not in), or where a function's offline
   copy is found removable; analysis-only code does not count in the unit's
   size for the growth budgets, and gets no IPA-CP or IPA-SRA clone, is not
   split and is merged with nothing by identical code folding (each would
   take a function number or change a symbol); and the clones the decisions
   make for the real callers leave its calls with the original.  The
   analysis-only code itself is optimized as before.  A shadow takes no
   clone number of its function's name and gives back its function number
   (process_function).  Its references still count (GCC-995, a documented
   limitation: DECISIONS.md E204 (a)).

   Premise copies (-fcontracts-prove-callee-premises, DECISIONS.md E199 (b),
   D4405R0 Section 4.5).  An observe, noexcept_observe or ignore evaluation
   is a premise for a later proof, which it makes conditional; but a
   shadow's callees are the real functions, whose regions are gone before
   anything is inlined, so a premise in a callee is lost.  The front end's
   strategy notes the regions such a copy can use
   (contract_shadow_note_premise_region), and contract_shadow_clone then
   makes, before it folds anything, a premise copy (suffix
   contract_premise) of each function holding an ordinary assumption region
   that is not guaranteed, which the direct calls of a function holding an
   ordinary obligation reach, transitively, and of each function such a
   call reaches one through: the copy executes the ordinary assumption
   regions only, like a shadow, holds no obligation, and calls the copies
   (make_premise_copies).  Each ordinary shadow whose function calls such a
   function gets a premise shadow (suffix contract_premise_shadow): another
   shadow of the body that calls the copies.  The copies and premise
   shadows are analysis-only code, so IPA's decisions about the real code
   are unchanged, and the copies are local and not forced, so that what the
   premise shadows do not inline is gone with them after inlining (under
   -flto, contract_shadow_early removes them).  The ordinary shadows are
   decided first, exactly as without the copies; then each premise shadow
   whose ordinary shadow left an obligation unproven or always false --
   the others are discarded unoptimized (skip_retry) -- and an obligation
   it proves becomes conditional (settle_retry): not diagnosed, never
   assumed.  A sound proof is never touched, so -fcontracts-prove-assume
   assumes the same with and without the copies, and the real code is the
   same.

   The proof optimization level (D4405, DECISIONS.md E162-E168, E206).  The
   analysis-only functions -- shadows of every kind and the copies -- are
   optimized at the proof level, whatever their functions' own levels: each
   gets the options the front end builds for that level from the command
   line's, with only the level replaced (contract_shadow_proof_optimization;
   without them, the function's own), and loses the function's optimize
   attribute (set_proof_options, shadow_attributes).  The inliner does not
   inline a function optimized at another level into them after the early
   optimizations, so a function a shadow may reach that is not at the proof
   level gets a level copy (suffix contract_level): a version of its body at
   the proof level, folded as the real function is, which the shadows and
   the other copies call instead -- a premise shadow and a premise copy
   call a premise copy first (make_level_copies).  Level copies are
   analysis-only code like the premise copies, local, not forced and gone
   with the shadows.  Where the levels agree, as by default at -O2 and -O3,
   none is made.  At proof level 0 no shadow is made unless a strategy is
   given explicitly, and then it is optimized at level 0, as at -O0
   (DECISIONS.md E208 (a)).  The shadows are expanded first, so the
   target's RTL machinery, set up with the options of the first function
   expanded, is set up before them, with the command line's
   (contract_shadow_expand_first).

   A -O0 compilation is no different (contract_shadow_level_p): its
   shadows and copies are optimized at the proof level, and the real code
   is still the same as without them.  Three things are needed that an
   optimized compilation does anyway, each only for the analysis-only
   code: the bodies of external functions (an `extern template' class's
   inline members, say) are analyzed and kept until the copies are made,
   for the shadows to inline, and dropped after
   (contract_shadow_external_bodies_p); the proof level's options let the
   shadows inline (the front end resets -O0's no inlining); and IPA
   pure-const runs for the functions optimized at the proof level
   (contract_analysis_ipa_p).  And since -O0 removes nothing, nothing of
   the regions may be left in the real function: the front end makes an
   `if' holding nothing but regions a region of its own, giving the real
   function the statement without them, as it is without the shadows
   (c_contract_hoist_region_guards); the assignments standing in for the
   guards, the temporaries only the regions used and the locations of the
   edges the regions leave go with the regions (fold_regions).  Without a
   proof level -- with no front end that gives one -- a -O0 compilation
   makes no shadow at all.  Under -fcontracts-prove-assume the assumptions
   made of the proofs are dropped with every other non-trivial assumption
   at -O0 (DECISIONS.md E167).

   Nor does the analysis-only code change the debug information (GCC-997).
   Versioning a function, or inlining one, makes dwarf2out describe it as
   an abstract instance with a concrete out-of-line one, and marks it
   possibly inlined; analysis-only code is never described, so a version
   made for it (analysis_version) and a body inlined into it
   (contract_analysis_only_inline_p, tree-inline.cc and
   ipa-inline-transform.cc) leave their functions as they are without it.
   Nor do the regions count where the lines recurring in different blocks
   are told apart by discriminators: assign_discriminators (tree-cfg.cc)
   numbers the real code's blocks as they are without the regions
   (contract_shadow_discriminator_kinds).

   The diagnostics are issued in registration order at the end of the
   compilation (contract_shadow_finish), so that an error found in one
   shadow cannot stop the passes that optimize another; so are those of
   the evaluations a front-end strategy decided while shadows may be
   active (contract_shadow_defer_diagnostic), since the middle end does not
   run at all after an error.  Each is issued in the context its evaluation
   was described in: in the function holding it ("In function ..."), and
   in whatever context the front end kept for it
   (contract_shadow_diagnostic_context: C++'s instantiation backtrace).

   An obligation may be nested in another (DECISIONS.md E169): the analysis
   evaluation of a precondition of a call in the other's predicate, which
   the other depends on whether or not its predicate is ever evaluated at
   run time.  Its marker is in the other's region, ahead of the call in
   the predicate, so the shadow decides it where the call is evaluated:
   after the conditions guarding the evaluation, and before anything the
   callee does -- undefined behavior in it included -- can make the path
   unreachable.  A front-end strategy may decide a nested evaluation at
   once instead, which is recorded with its result
   (contract_shadow_record_nested).  A nested obligation with the prove
   semantic that is not proven makes every obligation it is nested in
   unproven, and one proven only conditionally makes them conditional, so
   that no assumption is made of them; it is not diagnosed on its own, but
   noted with their diagnostics.  One with diagnose is diagnosed on its own,
   and one not proven soundly makes those it is nested in conditional
   (E169 (10)).  A nested obligation's region never joins the path again,
   so its failure is no premise for the other proofs in its shadow (E169
   (12)).  An obligation never evaluated, nothing nested in it being
   evaluated either, makes nothing nested in it count.  A nested diagnose
   obligation made for a contract assertion's analysis evaluation and one
   made for the runtime check of the same assertion, at the same call, are
   diagnosed once (E169 (9), contract_shadow_set_report_once).

   Obligations nothing decided -- in a function that is never compiled (an
   unused inline function), one that cannot be shadowed, or a compilation
   that stopped at an error -- are diagnosed as unproven, as without the
   shadow.  One in a function that was compiled but whose region was
   removed as unreachable before the shadow was made is never evaluated, so
   it is proven.

   Override shadows (D4405R0, "Virtual Functions").  For a virtual function
   D::f defined here and a function B::f it overrides, the C++ front end
   registers an override pair (contract_shadow_register_override) and
   places in D::f's body regions of that pair's kind: B::f's preconditions
   as assumptions, and as obligations, which the pair's override shadow
   decides, D::f's own preconditions at entry, its postconditions at each
   return and B::f's postconditions after them, with the snapshots they
   read assigned in regions of their own.  Every region names its kind -- the
   argument of .CONTRACT_SHADOW_GUARD, and of .CONTRACT_ASSUMPTION -- 0 for
   an ordinary region, active in every shadow of the function, and the
   pair's number for an override region, active only in that pair's
   shadow.  contract_shadow_clone makes one shadow per kind among the
   function's obligations: the ordinary shadow, as before, and an override
   shadow (suffix contract_override) per pair.  In an override shadow an
   ordinary obligation is no obligation -- it is decided by the ordinary
   shadow, and deciding it again would merge in the result of a proof made
   under B::f's preconditions -- but a premise, like any marker, and an
   override proof relying on one that is not proven soundly is only
   conditional; so the ordinary shadows are decided first.  An override
   obligation never becomes an assumption in the real code: the real D::f
   also serves calls that never established B::f's preconditions.  Its
   diagnostic names B::f, and is issued only if D::f is compiled here (an
   inline function whose vtable another translation unit emits is analyzed
   there).  Without override shadows (-fno-contracts-prove-override), the
   front end makes B::f's postconditions ordinary obligations of D::f,
   decided by its ordinary shadow, but registered with the pair for their
   diagnostic (obligation::named), which is issued in the same way.  After
   an error, when contract_shadow_clone never runs, "compiled here" is read
   from the symbol table's definitions instead, so these diagnostics are
   issued as ordinary ones are.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "tree.h"
#include "gimple.h"
#include "cfghooks.h"
#include "tree-pass.h"
#include "ssa.h"
#include "cgraph.h"
#include "diagnostic.h"
#include "fold-const.h"
#include "stringpool.h"
#include "attribs.h"
#include "gimple-iterator.h"
#include "tree-cfg.h"
#include "tree-cfgcleanup.h"
#include "tree-ssa.h"
#include "cfgloop.h"
#include "gimple-range.h"
#include "value-range.h"
#include "tree-inline.h"
#include "calls.h"
#include "options.h"
#include "varasm.h"
#include "debug.h"
#include "toplev.h"
#include "contracts-shadow.h"

namespace {

/* One obligation.  */

struct obligation
{
  location_t loc;
  location_t pred_loc;
  /* DECL_UID of the function whose body holds it.  */
  int fn_uid;
  bool prove;
  /* The predicate may have side effects other than through the calls in
     obligation_callees[first_callee .. first_callee + n_callees).  */
  bool side_effects;
  unsigned first_callee;
  unsigned n_callees;
  /* Its marker was found in a function's body.  */
  bool seen;
  /* That function could not be shadowed.  */
  bool no_shadow;
  contract_shadow_result result;
  /* The front end's context for its diagnostic; 0 for none.  */
  unsigned context;
  /* 0, or the override pair whose override shadow decides it.  */
  unsigned pair;
  /* 0, or the override pair for which the evaluation is made, whose
     overridden function its diagnostic names: PAIR, unless the evaluation
     is made in the ordinary shadow (-fno-contracts-prove-override), where
     PAIR is 0.  */
  unsigned named;
  /* 0, or one more than the id of the obligation it is nested in
     (DECISIONS.md E169).  */
  unsigned parent;
  /* Its marker, and those of every obligation nested in it, were removed
     as unreachable before the shadow was made: it is never evaluated, and
     nothing nested in it counts.  */
  bool unreached;
  /* The notes of a diagnostic decided at once:
     obligation_notes[first_note .. first_note + n_notes).  */
  unsigned first_note;
  unsigned n_notes;
  /* A front-end strategy found its predicate false wherever it is
     evaluated (DECISIONS.md E195): not proven, it is "always false"; it
     is proven only where it is never reached.  */
  bool folded_false;
  /* E169 (9): 0, or the evaluation of a contract assertion it is made for,
     with its analysis evaluation and its runtime check
     (contract_shadow_set_report_once); ONCE_CHECK if it is made for the
     check.  */
  unsigned once_group;
  bool once_check;
};

/* A call site on the way from a shadow's body into code inlined into it:
   the function called (the DECL_UID of its DECL_ORIGIN), the call's locus,
   and whether it is the last site of its chain -- the call of the function
   whose evaluations the shadow may have taken a premise from.  */

struct inline_site
{
  int fn_uid;
  location_t locus;
  bool last;
};

/* A site the proof of soundly proven obligation ID relied on being
   inlined.  */

struct obligation_site
{
  unsigned id;
  inline_site site;
};

/* What a shadow holds.  */

struct shadow_info
{
  /* The obligations whose markers it holds, each once.  */
  auto_vec<unsigned> ids;
  /* Those of them with a marker inside a loop when the shadow was made:
     a copy of an iteration may then be proven and removed while another
     is reached unconditionally, so none is "always false".  */
  hash_set<int_hash<unsigned, -1u, -2u> > in_loop;
  /* Every proof in it is at most conditional.  */
  bool conditional;
  /* The callees (contract_shadow_register_assumption) of the active
     assumption regions it holds, each once: a proof in it is conditional
     unless all of them are free of side effects.  */
  auto_vec<unsigned> assumption_deps;
  /* The chains of call sites through which it inlined code holding
     evaluations another translation unit's copy need not make
     (contract_shadow_note_inline).  */
  auto_vec<inline_site> sites;
  /* Whether it relies on a premise this TU does not decide
     (undecided_premise_p), once known: that is found when its late
     optimizations begin, while the call graph edges still exist.  */
  bool undecided_known;
  bool undecided;
  /* 0 for the ordinary shadow, or the override pair it is the override
     shadow of.  */
  unsigned pair;
  /* In an override shadow, the ordinary obligations whose markers it took
     as premises.  */
  auto_vec<unsigned> relied;
  /* For a premise shadow (-fcontracts-prove-callee-premises), one more
     than the index of the ordinary shadow whose failures it retries; 0 for
     any other shadow.  */
  unsigned premise_of;
};

/* What one shadow decided about one obligation it holds, kept while
   premise copies are made, so that a premise shadow can change what the
   ordinary shadow it retries decided (settle_retry).  */

struct shadow_decision
{
  unsigned id;
  unsigned shadow;
  contract_shadow_result result;
};

/* Every obligation, in registration (source) order.  */
static vec<obligation> obligations;
/* The ids of the obligations nested in another, in order.  */
static vec<unsigned> nested_ids;
/* The notes of the diagnostics decided at once (obligation::first_note).  */
static vec<contract_shadow_note> obligation_notes;
/* Every shadow, indexed by the value of its "contract shadow" attribute.  */
static vec<shadow_info *> shadows;
/* Shadows not yet diagnosed.  */
static unsigned live_shadows;
/* Set while the inliner's first removal after inlining runs
   (contract_shadow_boundary_hold).  */
static bool boundary_held;
/* Regions placed by the front ends.  */
static unsigned regions;
/* contract_shadow_clone removed regions from the real code.  */
static bool regions_removed;
/* .CONTRACT_PROVEN guards placed by the front ends and not yet folded
   everywhere.  */
static unsigned proven_guards;
/* contract_shadow_clone ran, and so found out which functions are
   compiled.  */
static bool compiled_known;
/* Ordinary assumption regions of evaluations that are not guaranteed,
   noted by -fcontracts-prove-callee-premises: premise copies are made only
   when there are some.  */
static unsigned premise_regions;
/* Premise copies made and not yet released (contract_shadow_finish).  */
static unsigned premise_copies;
/* Level copies made and not yet released.  */
static unsigned level_copies;
/* While premise copies are made, every result a shadow merged into an
   obligation's, in order.  */
static vec<shadow_decision> shadow_decisions;
/* Per contract_shadow_register_assumption, less one, the slice of
   obligation_callees holding the functions an assumption region's predicate
   calls: the first index and the count.  */
static vec<std::pair<unsigned, unsigned> > assumption_callees;
/* What contract_shadow_set_report_once set, for the obligations registered
   now.  */
static unsigned report_once_group;
static bool report_once_check;

/* The internal attribute marking a shadow.  The space keeps it out of
   reach of source code.  */
static const char shadow_attr[] = "contract shadow";
/* Likewise for a premise copy.  */
static const char premise_attr[] = "contract premise";
/* Likewise for a level copy.  */
static const char level_attr[] = "contract level";

} // anon namespace

/* Per obligation, the function whose body holds it, for the context of its
   diagnostic.  */
static GTY(()) vec<tree, va_gc> *obligation_fns;
/* The functions the obligations' and the assumption regions' predicates
   call, kept until the shadows holding them are decided.  */
static GTY(()) vec<tree, va_gc> *obligation_callees;
/* Per shadow, as for shadows, the function it is a copy of.  */
static GTY(()) vec<tree, va_gc> *shadow_real_fns;
/* Per override pair, the overridden function; element 0 is unused.  */
static GTY(()) vec<tree, va_gc> *override_bases;
/* The optimization options of the analysis-only functions at the proof
   level, while they are made (contract_shadow_clone).  */
static GTY(()) tree proof_optimization;

/* Premises decided by this translation unit.  A premise taken from a
   contract evaluation -- the code of a terminating check, or an
   assumption -- holds only where this translation unit's code for it is
   what runs: in the function's own body, where the shadow's assumption
   regions come from, or in code inlined into it.  A function another
   translation unit may provide (inline, comdat, weak, interposable) may be
   linked from a copy compiled with other semantics, so what its body lets
   a shadow conclude counts only where the real function inlined that same
   call too, and a call the shadow makes out of line to such a function
   must not let IPA summaries carry those evaluations' consequences in.
   Evaluations of an assertion whose allowed set admits no continuing
   semantic are not noted at all: every copy makes them.  */

/* DECL_UIDs of the DECL_ORIGINs of the functions whose bodies hold an
   evaluation noted by contract_shadow_note_premise, or code inlined from
   one before it reached a shadow.  */
static hash_set<int_hash<int, -1, -2> > *premise_fns;
/* The sites each soundly proven obligation's proof relied on.  */
static vec<obligation_site> obligation_sites;
/* DECL_UID of each -fcontracts-prove-assume assumption function -> the
   obligation whose .CONTRACT_PROVEN guard it holds.  */
static hash_map<int_hash<int, -1, -2>, unsigned> *assumption_fns;
/* Per cgraph uid, whether a call out of line to that node may let IPA
   summaries carry noted evaluations in (out_of_line_premises_p); valid
   while the shadows are decided.  */
static hash_map<int_hash<int, -1, -2>, bool> *premise_memo;

unsigned
contract_shadow_register_obligation (tree fndecl, location_t loc,
				     location_t pred_loc, bool prove,
				     bool side_effects,
				     const vec<tree> &callees,
				     unsigned context, unsigned named,
				     unsigned pair, unsigned parent,
				     bool always_false)
{
  obligation o = {};
  o.folded_false = always_false;
  o.context = context;
  o.pair = pair;
  o.named = named;
  o.parent = parent;
  if (parent)
    nested_ids.safe_push (obligations.length ());
  o.loc = loc;
  o.pred_loc = pred_loc;
  o.fn_uid = DECL_UID (DECL_ORIGIN (fndecl));
  o.prove = prove;
  o.side_effects = side_effects;
  o.first_callee = vec_safe_length (obligation_callees);
  o.n_callees = callees.length ();
  for (tree callee : callees)
    vec_safe_push (obligation_callees, callee);
  o.result = CSR_PENDING;
  o.once_group = report_once_group;
  o.once_check = report_once_check;
  obligations.safe_push (o);
  vec_safe_push (obligation_fns, DECL_ORIGIN (fndecl));
  ++regions;
  return obligations.length () - 1;
}

unsigned
contract_shadow_register_override (tree, tree base_fn)
{
  if (vec_safe_is_empty (override_bases))
    vec_safe_push (override_bases, NULL_TREE);
  vec_safe_push (override_bases, base_fn);
  return override_bases->length () - 1;
}

void
contract_shadow_note_region ()
{
  ++regions;
}

unsigned
contract_shadow_register_assumption (const vec<tree> &callees)
{
  gcc_checking_assert (!callees.is_empty ());
  assumption_callees.safe_push (std::make_pair (vec_safe_length
						  (obligation_callees),
						callees.length ()));
  for (tree callee : callees)
    vec_safe_push (obligation_callees, callee);
  return assumption_callees.length ();
}

void
contract_shadow_note_premise_region ()
{
  ++premise_regions;
}

void
contract_shadow_note_proven_guard ()
{
  ++proven_guards;
}

unsigned
contract_shadow_record_nested (tree fndecl, location_t loc,
			       location_t pred_loc, bool prove,
			       contract_shadow_result r, unsigned context,
			       unsigned parent, unsigned named)
{
  gcc_checking_assert (parent && parent <= obligations.length ());
  obligation o = {};
  o.context = context;
  o.loc = loc;
  o.pred_loc = pred_loc;
  o.fn_uid = -1;
  o.prove = prove;
  o.seen = true;
  o.parent = parent;
  o.named = named;
  o.result = r;
  o.once_group = report_once_group;
  o.once_check = report_once_check;
  nested_ids.safe_push (obligations.length ());
  obligations.safe_push (o);
  vec_safe_push (obligation_fns,
		 fndecl && TREE_CODE (fndecl) == FUNCTION_DECL
		 ? DECL_ORIGIN (fndecl) : NULL_TREE);
  return obligations.length () - 1;
}

void
contract_shadow_defer_diagnostic (tree fndecl, location_t loc,
				  location_t pred_loc, bool prove,
				  bool always_false, unsigned context,
				  unsigned pair,
				  const vec<contract_shadow_note> &notes)
{
  obligation o = {};
  o.context = context;
  o.loc = loc;
  o.pred_loc = pred_loc;
  o.fn_uid = -1;
  o.prove = prove;
  o.seen = true;
  o.pair = pair;
  o.named = pair;
  o.result = always_false ? CSR_ALWAYS_FALSE : CSR_UNPROVEN;
  o.once_group = report_once_group;
  o.once_check = report_once_check;
  o.first_note = obligation_notes.length ();
  o.n_notes = notes.length ();
  for (const contract_shadow_note &n : notes)
    obligation_notes.safe_push (n);
  if (pair && fndecl && TREE_CODE (fndecl) == FUNCTION_DECL)
    {
      /* Diagnosed only if the function is compiled, which
	 contract_shadow_clone finds out.  */
      o.fn_uid = DECL_UID (DECL_ORIGIN (fndecl));
      o.seen = false;
      ++regions;
    }
  obligations.safe_push (o);
  vec_safe_push (obligation_fns,
		 fndecl && TREE_CODE (fndecl) == FUNCTION_DECL
		 ? DECL_ORIGIN (fndecl) : NULL_TREE);
}

tree (*contract_shadow_proof_optimization) ();

/* True if the command line optimizes the translation unit.  */

static bool
optimized_unit_p ()
{
  return TREE_OPTIMIZATION (optimization_default_node)->x_optimize > 0;
}

bool
contract_shadow_level_p ()
{
  return optimized_unit_p () || contract_shadow_proof_optimization;
}

bool
contract_shadow_external_bodies_p ()
{
  return (regions
	  && !compiled_known
	  && contract_shadow_proof_optimization
	  && !optimized_unit_p ());
}

bool
contract_analysis_ipa_p ()
{
  return ((live_shadows || premise_copies || level_copies)
	  && !optimized_unit_p ());
}

bool
contract_shadow_pending_p ()
{
  for (const obligation &o : obligations)
    if (o.result == CSR_PENDING)
      return true;
  return false;
}

void
contract_shadow_set_report_once (unsigned group, bool check)
{
  report_once_group = group;
  report_once_check = check;
}

/* True if obligation D is nested in obligation X, through obligations
   none of which is unreached (X included): its result then counts for
   X's.  */

static bool
counts_for_p (unsigned d, unsigned x)
{
  for (unsigned p = obligations[d].parent; p; p = obligations[p - 1].parent)
    {
      if (obligations[p - 1].unreached)
	return false;
      if (p - 1 == x)
	return true;
    }
  return false;
}

/* The result of obligation ID with those nested in it (E169): one nested
   with prove not proven (or never decided) makes it unproven, one proven
   only conditionally makes a sound one conditional.  One nested with
   diagnose that is not proven soundly makes a sound one conditional too
   (E169 (10)), so that no assumption is made of it.  */

static contract_shadow_result
effective_result (unsigned id)
{
  contract_shadow_result r = obligations[id].result;
  if (r != CSR_SOUND && r != CSR_CONDITIONAL)
    return r;
  for (unsigned d : nested_ids)
    if (counts_for_p (d, id))
      switch (obligations[d].result)
	{
	case CSR_SOUND:
	  break;
	case CSR_CONDITIONAL:
	  r = CSR_CONDITIONAL;
	  break;
	default:
	  if (obligations[d].prove)
	    return CSR_UNPROVEN;
	  r = CSR_CONDITIONAL;
	  break;
	}
  return r;
}

/* True if none of the obligations obligation ID is nested in is
   unreached.  */

static bool
reached_p (unsigned id)
{
  for (unsigned p = obligations[id].parent; p; p = obligations[p - 1].parent)
    if (obligations[p - 1].unreached)
      return false;
  return true;
}

/* True if an obligation counts for obligation ID (counts_for_p).  */

static bool
has_nested_p (unsigned id)
{
  for (unsigned d : nested_ids)
    if (counts_for_p (d, id))
      return true;
  return false;
}

contract_shadow_result
contract_shadow_obligation_result (unsigned id)
{
  gcc_checking_assert (id < obligations.length ());
  return effective_result (id);
}

bool
contract_shadow_function_p (const_tree fndecl)
{
  return (!shadows.is_empty ()
	  && lookup_attribute (shadow_attr, DECL_ATTRIBUTES (fndecl)));
}

bool
contract_shadow_boundary_p (cgraph_node *node)
{
  if (!live_shadows || boundary_held
      || symtab->state < IPA_SSA_AFTER_INLINING)
    return false;
  if (node->inlined_to)
    node = node->inlined_to;
  return contract_shadow_function_p (node->decl);
}

bool
contract_shadow_boundary_hold (bool hold)
{
  boundary_held = hold;
  return live_shadows != 0;
}

/* True if FNDECL is a premise copy (-fcontracts-prove-callee-premises).  */

static bool
premise_copy_p (const_tree fndecl)
{
  return (premise_copies
	  && lookup_attribute (premise_attr, DECL_ATTRIBUTES (fndecl)));
}

/* True if FNDECL is a level copy (a callee of a shadow at the proof
   level).  */

static bool
level_copy_p (const_tree fndecl)
{
  return (level_copies
	  && lookup_attribute (level_attr, DECL_ATTRIBUTES (fndecl)));
}

/* True if FNDECL is analysis-only code in its own right: a proof shadow, a
   premise shadow included, a premise copy or a level copy.  */

static bool
analysis_only_fn_p (const_tree fndecl)
{
  return (contract_shadow_function_p (fndecl) || premise_copy_p (fndecl)
	  || level_copy_p (fndecl));
}

/* GCC-907: what the analysis-only code -- the proof shadows -- is to IPA's
   decisions about the real code.  What NODE's existence and its calls and
   references come to: REAL if some code that is not analysis-only (or
   something outside the translation unit) needs it, ANALYSIS if only
   analysis-only code does, NONE if nothing does (dead code, as dead with
   or without the shadows).  VISITING holds the symbols whose callers are
   being followed, so that a cycle adds nothing.  */

enum analysis_need { NEED_REAL, NEED_ANALYSIS, NEED_NONE };

static analysis_need
analysis_need_of (symtab_node *node, hash_set<symtab_node *> &visiting)
{
  if (cgraph_node *cnode = dyn_cast <cgraph_node *> (node))
    {
      if (cnode->inlined_to)
	return analysis_need_of (cnode->inlined_to, visiting);
      if (analysis_only_fn_p (cnode->decl))
	return NEED_ANALYSIS;
      if (!cnode->can_remove_if_no_direct_calls_and_refs_p ())
	return NEED_REAL;
    }
  else if (varpool_node *vnode = dyn_cast <varpool_node *> (node))
    {
      if (!vnode->can_remove_if_no_refs_p ())
	return NEED_REAL;
    }
  else
    return NEED_REAL;
  if (visiting.add (node))
    return NEED_NONE;

  analysis_need need = NEED_NONE;
  if (cgraph_node *cnode = dyn_cast <cgraph_node *> (node))
    for (cgraph_edge *e = cnode->callers; e; e = e->next_caller)
      switch (analysis_need_of (e->caller, visiting))
	{
	case NEED_REAL:
	  return NEED_REAL;
	case NEED_ANALYSIS:
	  need = NEED_ANALYSIS;
	  break;
	case NEED_NONE:
	  break;
	}
  ipa_ref *ref;
  for (unsigned i = 0; node->iterate_referring (i, ref); i++)
    switch (analysis_need_of (ref->referring, visiting))
      {
      case NEED_REAL:
	return NEED_REAL;
      case NEED_ANALYSIS:
	need = NEED_ANALYSIS;
	break;
      case NEED_NONE:
	break;
      }
  return need;
}

bool
contract_analysis_only_p (symtab_node *node)
{
  if (!live_shadows)
    return false;
  /* The common answers without a walk.  */
  if (cgraph_node *cnode = dyn_cast <cgraph_node *> (node))
    {
      if (cnode->inlined_to)
	cnode = cnode->inlined_to;
      if (analysis_only_fn_p (cnode->decl))
	return true;
      if (!cnode->can_remove_if_no_direct_calls_and_refs_p ())
	return false;
    }
  else if (!is_a <varpool_node *> (node))
    return false;
  hash_set<symtab_node *> visiting;
  return analysis_need_of (node, visiting) == NEED_ANALYSIS;
}

bool
contract_analysis_only_definition_p (symtab_node *node)
{
  /* Without a shadow, a region removed from the real code may still have
     made a definition of what its predicate calls, which nothing real
     needs (an ignore contract_assert's).  */
  if (!live_shadows && !regions_removed)
    return false;
  cgraph_node *cur
    = current_function_decl ? cgraph_node::get (current_function_decl) : NULL;
  if (cur && contract_analysis_only_p (cur))
    return false;
  hash_set<symtab_node *> visiting;
  if (analysis_need_of (node, visiting) == NEED_REAL)
    return false;
  /* With -fdevirtualize the possible targets of the real code's polymorphic
     calls are kept for it (analyze_functions, remove_unreachable_nodes),
     with or without the shadows.  Without it, a function only a shadow or
     a copy of it calls -- even one no code calls any longer, its copy
     calling in its place -- would not be defined here.  */
  if (DECL_VIRTUAL_P (node->decl)
      && (cur ? opt_for_fn (cur->decl, flag_devirtualize) : flag_devirtualize))
    return false;
  return true;
}

bool
contract_analysis_only_call_p (cgraph_edge *e)
{
  return (live_shadows
	  && contract_analysis_only_p (e->caller)
	  && !contract_analysis_only_p (e->callee->ultimate_alias_target ()));
}

bool
contract_analysis_only_inline_p (cgraph_node *dst)
{
  return live_shadows && contract_analysis_only_p (dst);
}

/* True if block B ends in the condition on a region's guard, the
   statement before it: `G = .CONTRACT_SHADOW_GUARD (...); if (G != 0)'.  */

static bool
guard_block_p (basic_block b)
{
  gimple_stmt_iterator gsi = gsi_last_bb (b);
  gcond *c = gsi_end_p (gsi) ? NULL : dyn_cast <gcond *> (gsi_stmt (gsi));
  if (!c)
    return false;
  gsi_prev (&gsi);
  if (gsi_end_p (gsi)
      || !gimple_call_internal_p (gsi_stmt (gsi), IFN_CONTRACT_SHADOW_GUARD))
    return false;
  tree lhs = gimple_call_lhs (gsi_stmt (gsi));
  return lhs && gimple_cond_lhs (c) == lhs;
}

/* Set REACHED to the blocks reached from START, in the current function,
   without passing through BARRIER.  */

static void
reached_blocks (basic_block start, basic_block barrier, sbitmap reached)
{
  bitmap_clear (reached);
  auto_vec<basic_block> work;
  if (start != barrier && start->index >= NUM_FIXED_BLOCKS)
    {
      bitmap_set_bit (reached, start->index);
      work.safe_push (start);
    }
  while (!work.is_empty ())
    {
      basic_block b = work.pop ();
      for (edge e : b->succs)
	if (e->dest != barrier && e->dest->index >= NUM_FIXED_BLOCKS
	    && bitmap_set_bit (reached, e->dest->index))
	  work.safe_push (e->dest);
    }
}

bool
contract_shadow_discriminator_kinds (vec<unsigned char> &kinds)
{
  if (!regions)
    return false;
  auto_vec<basic_block> guards;
  basic_block b;
  FOR_EACH_BB_FN (b, cfun)
    if (guard_block_p (b))
      guards.safe_push (b);
  if (guards.is_empty ())
    return false;

  /* A region's blocks are those its guard's true edge reaches and its
     false edge does not.  */
  unsigned n = last_basic_block_for_fn (cfun);
  kinds.safe_grow_cleared (n);
  auto_sbitmap on_true (n), on_false (n);
  for (basic_block g : guards)
    {
      edge t, f;
      extract_true_false_edges_from_block (g, &t, &f);
      reached_blocks (t->dest, g, on_true);
      reached_blocks (f->dest, g, on_false);
      bitmap_and_compl (on_true, on_true, on_false);
      sbitmap_iterator sbi;
      unsigned i;
      EXECUTE_IF_SET_IN_BITMAP (on_true, 0, i, sbi)
	kinds[i] = CONTRACT_DISCRIM_REGION;
    }

  /* Without the region, the guard's block would go on into the false
     edge's destination if that comes next and nothing else enters it.  */
  for (basic_block g : guards)
    {
      if (kinds[g->index] == CONTRACT_DISCRIM_REGION)
	continue;
      edge t, f;
      extract_true_false_edges_from_block (g, &t, &f);
      basic_block next = g->next_bb;
      while (next->index >= NUM_FIXED_BLOCKS
	     && kinds[next->index] == CONTRACT_DISCRIM_REGION)
	next = next->next_bb;
      bool joined = f->dest == next;
      for (edge e : f->dest->preds)
	if (e->src != g
	    && (e->src->index < NUM_FIXED_BLOCKS
		|| kinds[e->src->index] != CONTRACT_DISCRIM_REGION))
	  joined = false;
      kinds[g->index] = (joined ? CONTRACT_DISCRIM_GUARD_JOINED
			 : CONTRACT_DISCRIM_GUARD);
    }
  return true;
}

void
contract_shadow_switch_function (tree fndecl)
{
  static bool inhibiting, saved;
  bool shadow = (live_shadows && fndecl
		 && contract_shadow_function_p (fndecl));
  if (shadow == inhibiting)
    return;
  if (shadow)
    {
      saved = global_dc->m_inhibit_warnings;
      global_dc->m_inhibit_warnings = true;
    }
  else
    global_dc->m_inhibit_warnings = saved;
  inhibiting = shadow;
}

void
contract_shadow_note_premise (tree fndecl)
{
  if (!premise_fns)
    premise_fns = new hash_set<int_hash<int, -1, -2> >;
  premise_fns->add (DECL_UID (DECL_ORIGIN (fndecl)));
}

namespace {

/* True if the body of FNDECL holds an evaluation noted by
   contract_shadow_note_premise, or code inlined from one.  */

static bool
premise_fn_p (tree fndecl)
{
  return (premise_fns
	  && premise_fns->contains (DECL_UID (DECL_ORIGIN (fndecl))));
}

/* True if the definition of FNDECL the program runs may be another
   translation unit's: its own is not the one linked (an extern inline or C
   inline definition, an explicit instantiation declaration's), or it is
   comdat, weak, or interposable.  */

static bool
replaceable_p (tree fndecl)
{
  if (DECL_EXTERNAL (fndecl))
    return true;
  if (!TREE_PUBLIC (fndecl))
    return false;
  return (DECL_COMDAT (fndecl) || DECL_ONE_ONLY (fndecl) || DECL_WEAK (fndecl)
	  || decl_replaceable_p (fndecl, opt_for_fn (fndecl,
						     flag_semantic_interposition)));
}

/* Likewise, or FNDECL is a local copy (an IPA clone) of such a function:
   that copy is this translation unit's code, but where a shadow inlined
   it, the real function may call the function itself.  */

static bool
replaceable_origin_p (tree fndecl)
{
  if (replaceable_p (fndecl))
    return true;
  tree origin = DECL_ORIGIN (fndecl);
  return (origin != fndecl && !TREE_PUBLIC (fndecl) && TREE_PUBLIC (origin)
	  && replaceable_p (origin));
}

/* The function NODE is a copy of.  */

static tree
node_origin (cgraph_node *node)
{
  while (node->clone_of)
    node = node->clone_of;
  return DECL_ORIGIN (node->decl);
}

/* The locus of the call STMT of FN, as the inliner records it for the
   block of an inlined copy (expand_call_inline).  */

static location_t
call_locus (gimple *stmt, tree fn)
{
  location_t loc = LOCATION_LOCUS (gimple_location (stmt));
  if (loc == UNKNOWN_LOCATION && fn)
    loc = LOCATION_LOCUS (DECL_SOURCE_LOCATION (fn));
  if (loc == UNKNOWN_LOCATION)
    loc = BUILTINS_LOCATION;
  return loc;
}

/* The DECL_UID of the function whose inlined copy has its outermost scope
   in BLOCK, or -1 if BLOCK is not such a scope.  */

static int
inlined_fn_uid (tree block)
{
  if (!inlined_function_outer_scope_p (block))
    return -1;
  tree origin = block_ultimate_origin (block);
  if (!origin || TREE_CODE (origin) != FUNCTION_DECL)
    return -1;
  return DECL_UID (DECL_ORIGIN (origin));
}

/* True if STMT is within BLOCK.  */

static bool
within_block_p (gimple *stmt, tree block)
{
  for (tree b = gimple_block (stmt); b && TREE_CODE (b) == BLOCK;
       b = BLOCK_SUPERCONTEXT (b))
    if (b == block)
      return true;
  return false;
}

/* The shadow_info of shadow FNDECL.  */

static unsigned
shadow_index_of (tree fndecl)
{
  tree attr = lookup_attribute (shadow_attr, DECL_ATTRIBUTES (fndecl));
  gcc_checking_assert (attr);
  return tree_to_uhwi (TREE_VALUE (TREE_VALUE (attr)));
}

static shadow_info *
shadow_info_of (tree fndecl)
{
  return shadows[shadow_index_of (fndecl)];
}

static const char *
result_name (contract_shadow_result r)
{
  switch (r)
    {
    case CSR_PENDING: return "pending";
    case CSR_UNPROVEN: return "unproven";
    case CSR_ALWAYS_FALSE: return "always false";
    case CSR_CONDITIONAL: return "conditional";
    case CSR_SOUND: return "sound";
    }
  gcc_unreachable ();
}

/* Combine R, one copy's result, into obligation O's: unproven if any copy
   is, always false only if every copy is, conditional if any copy is.  */

static void
merge_result (obligation &o, contract_shadow_result r)
{
  contract_shadow_result old = o.result;
  if (old == CSR_PENDING)
    o.result = r;
  else if (old == CSR_ALWAYS_FALSE && r == CSR_ALWAYS_FALSE)
    ;
  else if (old == CSR_UNPROVEN || old == CSR_ALWAYS_FALSE
	   || r == CSR_UNPROVEN || r == CSR_ALWAYS_FALSE)
    o.result = CSR_UNPROVEN;
  else if (old == CSR_CONDITIONAL || r == CSR_CONDITIONAL)
    o.result = CSR_CONDITIONAL;
  else
    o.result = CSR_SOUND;
}

/* True if one of the N functions at obligation_callees[FIRST] is not
   known now to be const or pure, terminating and not throwing.  */

static bool
callees_side_effects_p (unsigned first, unsigned n)
{
  for (unsigned i = 0; i < n; ++i)
    {
      tree callee = (*obligation_callees)[first + i];
      int flags = flags_from_decl_or_type (callee);
      if (!(flags & (ECF_CONST | ECF_PURE))
	  || (flags & ECF_LOOPING_CONST_OR_PURE)
	  || (!(flags & ECF_NOTHROW) && flag_exceptions))
	return true;
    }
  return false;
}

/* True if evaluating the predicate of obligation O may have side effects,
   as far as is known now: it has some of its own, or it calls a function
   not known to be const or pure, terminating and not throwing.  */

static bool
side_effects_p (const obligation &o)
{
  return (o.side_effects
	  || callees_side_effects_p (o.first_callee, o.n_callees));
}

/* Likewise for the predicate of the assumption regions registered as DEP
   (contract_shadow_register_assumption), which has none of its own.  */

static bool
assumption_side_effects_p (unsigned dep)
{
  gcc_checking_assert (dep && dep <= assumption_callees.length ());
  const std::pair<unsigned, unsigned> &c = assumption_callees[dep - 1];
  return callees_side_effects_p (c.first, c.second);
}

/* If the current function, FNDECL, is the assumption function of an
   assumption made of an obligation (the gimplifier outlines its
   condition), record which obligation.  */

static void
note_assumption_fn (tree fndecl)
{
  if (!proven_guards)
    return;
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	 gsi_next (&gsi))
      if (gimple_call_internal_p (gsi_stmt (gsi), IFN_CONTRACT_PROVEN))
	{
	  if (!assumption_fns)
	    assumption_fns = new hash_map<int_hash<int, -1, -2>, unsigned>;
	  assumption_fns->put (DECL_UID (fndecl),
			       tree_to_uhwi (gimple_call_arg (gsi_stmt (gsi),
							      0)));
	  return;
	}
}

/* True if the current function holds a region.  */

static bool
has_regions_p ()
{
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	 gsi_next (&gsi))
      if (gimple_call_internal_p (gsi_stmt (gsi), IFN_CONTRACT_SHADOW_GUARD))
	return true;
  return false;
}

/* Replace the marker at GSI, in a real function, by a trap: the region
   holding it is removed below, so this only matters should its guard's
   condition not be found, and keeps it from ever being expanded.  */

static void
replace_with_trap (gimple_stmt_iterator *gsi)
{
  gimple *g = gimple_build_call (builtin_decl_implicit (BUILT_IN_TRAP), 0);
  gimple_set_location (g, gimple_location (gsi_stmt (*gsi)));
  gsi_replace (gsi, g, false);
}

/* Make the condition on each guard result in GUARDS, a map from the
   result to its value, true or false accordingly.  In the real function
   (REAL), the edges out of such a condition lose their locations too: they
   are the region's, and -O0 would mark the line of an edge left by the
   region's removal with a nop.  */

static void
fold_guard_conditions (hash_map<tree, bool> &guards, bool real = false)
{
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    if (gcond *c = safe_dyn_cast <gcond *> (*gsi_last_bb (bb)))
      {
	tree lhs = gimple_cond_lhs (c);
	bool *value = guards.get (lhs);
	if (!value)
	  continue;
	tree v = fold_binary (gimple_cond_code (c), boolean_type_node,
			      constant_boolean_node (*value, TREE_TYPE (lhs)),
			      gimple_cond_rhs (c));
	if (!v || TREE_CODE (v) != INTEGER_CST)
	  continue;
	if (integer_zerop (v))
	  gimple_cond_make_false (c);
	else
	  gimple_cond_make_true (c);
	update_stmt (c);
	if (real)
	  {
	    edge e;
	    edge_iterator ei;
	    FOR_EACH_EDGE (e, ei, bb->succs)
	      e->goto_locus = UNKNOWN_LOCATION;
	  }
      }
}

/* Replace the guard call at GSI by the assignment of VALUE to its result,
   recorded in GUARDS, or by nothing if it has none.  The assignment stands
   in for the call should its condition not be found by
   fold_guard_conditions; the optimizers then remove the region instead.  */

static void
replace_guard (gimple_stmt_iterator *gsi, bool value,
	       hash_map<tree, bool> &guards)
{
  gimple *stmt = gsi_stmt (*gsi);
  tree lhs = gimple_call_lhs (stmt);
  gimple *g;
  if (lhs)
    {
      guards.put (lhs, value);
      g = gimple_build_assign (lhs,
			       constant_boolean_node (value, TREE_TYPE (lhs)));
    }
  else
    g = gimple_build_nop ();
  gimple_set_location (g, gimple_location (stmt));
  gsi_replace (gsi, g, false);
}

/* The kind of the region STMT belongs to -- STMT is its guard, whose
   argument ARGNO is 0, or an assumption's tail, whose argument ARGNO is 1:
   0 for an ordinary region, an override pair's number for one of that
   pair's.  */

static unsigned
region_kind (gimple *stmt, unsigned argno)
{
  if (gimple_call_num_args (stmt) <= argno)
    return 0;
  tree a = gimple_call_arg (stmt, argno);
  return tree_fits_uhwi_p (a) ? tree_to_uhwi (a) : 0;
}

/* True if STMT, a guard, is an obligation's (CONTRACT_SHADOW_GUARD in
   internal-fn.def).  */

static bool
obligation_guard_p (gimple *stmt)
{
  return (gimple_call_num_args (stmt) > 1
	  && integer_onep (gimple_call_arg (stmt, 1)));
}

/* True if a region of kind KIND is executed in the shadow INFO: an ordinary
   region in every shadow, an override region only in its pair's.  */

static bool
region_active_p (unsigned kind, const shadow_info *info)
{
  return kind == 0 || kind == info->pair;
}

/* walk_tree callback: add each automatic artificial variable of the
   current function to the hash_set<tree> DATA points to.  */

static tree
collect_temporaries_r (tree *tp, int *walk_subtrees, void *data)
{
  tree t = *tp;
  if (VAR_P (t)
      && DECL_ARTIFICIAL (t)
      && !TREE_STATIC (t)
      && !DECL_EXTERNAL (t)
      && DECL_CONTEXT (t) == current_function_decl)
    ((hash_set<tree> *) data)->add (t);
  else if (TYPE_P (t) || DECL_P (t))
    *walk_subtrees = 0;
  return NULL_TREE;
}

/* Add to TEMPS each automatic artificial variable the current function's
   statements refer to.  */

static void
collect_temporaries (hash_set<tree> &temps)
{
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	 gsi_next (&gsi))
      for (unsigned i = 0; i < gimple_num_ops (gsi_stmt (gsi)); ++i)
	if (gimple_op (gsi_stmt (gsi), i))
	  walk_tree_without_duplicates (gimple_op_ptr (gsi_stmt (gsi), i),
					collect_temporaries_r, &temps);
}

/* In the real function, whose regions have just been removed: drop from
   its local declarations the temporaries that only the regions used --
   those in BEFORE, collected before the regions went, that nothing refers
   to now -- which -O0 would otherwise give stack slots (it removes no
   unused declaration).  A variable the program declared stays, as it
   does without the regions.  */

static void
drop_region_temporaries (hash_set<tree> &before)
{
  if (before.is_empty () || !cfun->local_decls)
    return;
  hash_set<tree> after;
  collect_temporaries (after);
  vec<tree, va_gc> *kept = NULL;
  bool dropped = false;
  for (tree var : *cfun->local_decls)
    if (before.contains (var) && !after.contains (var))
      {
	TREE_USED (var) = 0;
	dropped = true;
      }
    else
      vec_safe_push (kept, var);
  if (dropped)
    cfun->local_decls = kept;
}

/* walk_tree callback: count each occurrence of a guard's result, a key of
   the hash_map<tree, unsigned> DATA points to.  */

static tree
count_guard_uses_r (tree *tp, int *, void *data)
{
  if (unsigned *n = ((hash_map<tree, unsigned> *) data)->get (*tp))
    ++*n;
  return NULL_TREE;
}

/* In the real function, whose regions have just been removed: remove the
   assignments that stood in for the guards, whose results are the keys of
   GUARDS, where nothing reads them any more, so that nothing is left of the
   regions for -O0 to emit.  */

static void
remove_dead_guards (hash_map<tree, bool> &guards)
{
  hash_map<tree, unsigned> uses;
  for (auto g : guards)
    uses.put (g.first, 0);
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	 gsi_next (&gsi))
      for (unsigned i = 0; i < gimple_num_ops (gsi_stmt (gsi)); ++i)
	if (gimple_op (gsi_stmt (gsi), i))
	  walk_tree_without_duplicates (gimple_op_ptr (gsi_stmt (gsi), i),
					count_guard_uses_r, &uses);
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi); )
      {
	tree lhs = gimple_get_lhs (gsi_stmt (gsi));
	unsigned *n = lhs ? uses.get (lhs) : NULL;
	if (n && *n == 1 && is_gimple_assign (gsi_stmt (gsi)))
	  gsi_remove (&gsi, true);
	else
	  gsi_next (&gsi);
      }
}

/* Fold every region guard of the current function -- to false in the real
   function (INFO null), and in shadow INFO to whether the region is active
   there -- let the CFG cleanup remove what that makes unreachable, and
   rebuild the call graph edges.  In a shadow, the tails of the active
   assumptions become __builtin_unreachable (), and INFO records the
   obligations and whether the shadow is conditional; in an override shadow
   an ordinary obligation's marker becomes __builtin_unreachable () too, a
   premise, recorded in INFO's relied.  In the real function mark the
   obligations found as seen, and as not shadowed unless SHADOWED, the
   kinds that got a shadow, holds theirs, and remove what the regions leave
   that -O0 would emit: the assignments standing in for their guards, the
   temporaries only they used, the locations of the edges out of their
   guards.  In a copy (COPY, INFO null)
   nothing is recorded: in a premise copy only the ordinary assumption
   regions are executed, their tails becoming __builtin_unreachable (); a
   level copy is folded as the real function.  */

enum copy_kind { NOT_A_COPY, PREMISE_COPY, LEVEL_COPY };

static void
fold_regions (shadow_info *info, const vec<unsigned> &shadowed,
	      copy_kind kind = NOT_A_COPY)
{
  bool copy = kind == PREMISE_COPY;
  hash_map<tree, bool> guards;
  hash_set<tree> temporaries;
  if (!info && kind == NOT_A_COPY)
    collect_temporaries (temporaries);
  hash_set<int_hash<unsigned, -1u, -2u> > ids, relied;
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	 gsi_next (&gsi))
      {
	gimple *stmt = gsi_stmt (gsi);
	if (!is_gimple_call (stmt) || !gimple_call_internal_p (stmt))
	  continue;
	switch (gimple_call_internal_fn (stmt))
	  {
	  case IFN_CONTRACT_SHADOW_GUARD:
	    /* A nested obligation's guard is left unknown in a shadow
	       (CONTRACT_SHADOW_NESTED_KIND).  */
	    if (info && region_kind (stmt, 0) == CONTRACT_SHADOW_NESTED_KIND)
	      break;
	    if (copy)
	      replace_guard (&gsi,
			     region_kind (stmt, 0) == 0
			     && !obligation_guard_p (stmt),
			     guards);
	    else
	      replace_guard (&gsi,
			     info && region_active_p (region_kind (stmt, 0),
						      info),
			     guards);
	    break;
	  case IFN_CONTRACT_UNPROVEN:
	    {
	      unsigned id = tree_to_uhwi (gimple_call_arg (stmt, 0));
	      gcc_checking_assert (id < obligations.length ());
	      obligation &o = obligations[id];
	      if (kind != NOT_A_COPY)
		replace_with_trap (&gsi);
	      else if (!info)
		{
		  o.seen = true;
		  if (!shadowed.contains (o.pair))
		    o.no_shadow = true;
		  replace_with_trap (&gsi);
		}
	      else if (o.pair == info->pair)
		{
		  if (!ids.add (id))
		    info->ids.safe_push (id);
		  /* The marker's own block reaches no latch, being noreturn,
		     so it belongs to no loop; the condition guarding it
		     does.  */
		  bool in_loop = !current_loops;
		  edge e;
		  edge_iterator ei;
		  FOR_EACH_EDGE (e, ei, bb->preds)
		    if (!current_loops
			|| e->src->loop_father != current_loops->tree_root)
		      in_loop = true;
		  if (in_loop)
		    info->in_loop.add (id);
		}
	      else if (o.pair == 0)
		{
		  /* An ordinary obligation in an override shadow: the
		     ordinary shadow decides it; here what follows may assume
		     its predicate, as after any marker.  A nested one's
		     region never joins the path again
		     (CONTRACT_SHADOW_NESTED_KIND), so nothing relies on
		     it.  */
		  if (!o.parent && !relied.add (id))
		    info->relied.safe_push (id);
		  gimple *g
		    = gimple_build_builtin_unreachable (gimple_location (stmt));
		  gsi_replace (&gsi, g, false);
		}
	      else
		/* Another pair's: its region is removed below.  */
		replace_with_trap (&gsi);
	      break;
	    }
	  case IFN_CONTRACT_ASSUMPTION:
	    if (copy ? region_kind (stmt, 1) == 0
		: info && region_active_p (region_kind (stmt, 1), info))
	      {
		if (info && integer_zerop (gimple_call_arg (stmt, 0)))
		  info->conditional = true;
		/* The functions its predicate calls (none recorded in a
		   premise copy, whose proofs are all conditional).  */
		if (info && gimple_call_num_args (stmt) > 2)
		  {
		    unsigned dep = tree_to_uhwi (gimple_call_arg (stmt, 2));
		    if (dep && !info->assumption_deps.contains (dep))
		      info->assumption_deps.safe_push (dep);
		  }
		gimple *g
		  = gimple_build_builtin_unreachable (gimple_location (stmt));
		gsi_replace (&gsi, g, false);
	      }
	    else
	      replace_with_trap (&gsi);
	    break;
	  default:
	    break;
	  }
      }

  /* The guard's value decides the condition the gimplifier built on it.  */
  if (!guards.is_empty ())
    fold_guard_conditions (guards, !info && kind == NOT_A_COPY);

  free_dominance_info (CDI_DOMINATORS);
  free_dominance_info (CDI_POST_DOMINATORS);
  if (current_loops)
    loops_state_set (LOOPS_NEED_FIXUP);
  cleanup_tree_cfg ();
  if (!info && kind == NOT_A_COPY)
    {
      if (!guards.is_empty ())
	remove_dead_guards (guards);
      drop_region_temporaries (temporaries);
    }
  cgraph_edge::rebuild_edges ();
}

/* True if the current function's body can be copied into a shadow and the
   shadow optimized; NODE is its cgraph node.  */

static bool
shadowable_p (cgraph_node *node)
{
  tree decl = node->decl;
  return (contract_shadow_level_p ()
	  && !node->alias
	  && !node->thunk
	  && !node->parallelized_function
	  && !cfun->static_chain_decl
	  && !DECL_FUNCTION_VERSIONED (decl)
	  && !lookup_attribute ("target_clones", DECL_ATTRIBUTES (decl))
	  && !lookup_attribute ("target_version", DECL_ATTRIBUTES (decl))
	  && tree_versionable_function_p (decl));
}

/* The attributes of a shadow of a function with attributes ATTRS: those
   that describe the body, without those that would make the shadow
   visible, emitted, inlined or versioned, plus SHADOW_ATTR with value
   INDEX and what keeps the shadow to itself.  For a copy -- COPY_ATTR is
   then PREMISE_ATTR or LEVEL_ATTR, which replaces SHADOW_ATTR -- those that
   decide how it is inlined stay, as they are what the shadows see of the
   real function.  The function's optimize attribute goes: an
   analysis-only function is optimized at the proof level whatever the
   function's own level (DECISIONS.md E168 (7), set_proof_options).  */

static tree
shadow_attributes (tree attrs, unsigned index, const char *copy_attr = NULL)
{
  static const char *const dropped[] = {
    "always_inline", "noinline", "gnu_inline", "noclone", "no_icf",
    "target_clones", "target_version", "alias", "ifunc", "weak", "weakref",
    "used", "retain", "externally_visible", "symver", "constructor",
    "destructor", "omp declare simd", "omp declare target",
    "omp declare target block", "omp declare variant base", "optimize"
  };
  bool copy = copy_attr != NULL;
  tree result = NULL_TREE;
  for (tree a = attrs; a; a = TREE_CHAIN (a))
    {
      bool drop = false;
      /* A copy keeps the first two.  */
      for (unsigned i = copy ? 2 : 0; i < ARRAY_SIZE (dropped); ++i)
	if (is_attribute_p (dropped[i], get_attribute_name (a)))
	  drop = true;
      if (!drop)
	result = tree_cons (TREE_PURPOSE (a), TREE_VALUE (a), result);
    }
  result = nreverse (result);
  if (!copy)
    result = tree_cons (get_identifier ("noinline"), NULL_TREE, result);
  result = tree_cons (get_identifier ("noclone"), NULL_TREE, result);
  result = tree_cons (get_identifier ("no_icf"), NULL_TREE, result);
  if (copy)
    return tree_cons (get_identifier (copy_attr), NULL_TREE, result);
  result = tree_cons (get_identifier (shadow_attr),
		      build_tree_list (NULL_TREE,
				       build_int_cst (unsigned_type_node,
						      index)),
		      result);
  return result;
}

/* A version of NODE with its body, named with SUFFIX, which holds its own
   index, or NULL.  GCC-907: it takes neither a clone number of NODE's name
   nor a function number, so that the clones and functions made later are
   named and numbered (.LFB labels) as without it: as it is never emitted,
   the number it took is given back.  GCC-997: nor does it change NODE's
   debug information: versioning describes NODE as an abstract instance at
   once (the outlining_inline_function debug hook) and marks it possibly
   inlined, as for a version that is emitted.  */

static cgraph_node *
analysis_version (cgraph_node *node, const char *suffix)
{
  int funcdef_no = get_last_funcdef_no ();
  bool possibly_inlined = DECL_POSSIBLY_INLINED (node->decl);
  const gcc_debug_hooks *hooks = debug_hooks;
  debug_hooks = &do_nothing_debug_hooks;
  cgraph_node *version
    = node->create_version_clone_with_body (vNULL, NULL, NULL, NULL, NULL,
					    suffix, NULL_TREE, false);
  debug_hooks = hooks;
  DECL_POSSIBLY_INLINED (node->decl) = possibly_inlined;
  set_last_funcdef_no (funcdef_no);
  return version;
}

/* D4405's proof optimization level: give FNDECL, an analysis-only
   function, the options of the proof level in place of those of the
   function it copies, whatever that function's own optimize attribute
   (DECISIONS.md E168 (7)).  */

static void
set_proof_options (tree fndecl)
{
  if (!proof_optimization)
    return;
  DECL_FUNCTION_SPECIFIC_OPTIMIZATION (fndecl)
    = (proof_optimization == optimization_default_node
       ? NULL_TREE : proof_optimization);
}

/* True if NODE is optimized at the proof level, as an analysis-only function
   would be: a shadow may inline it as it is.  */

static bool
at_proof_level_p (cgraph_node *node)
{
  if (!proof_optimization)
    return true;
  tree opts = DECL_FUNCTION_SPECIFIC_OPTIMIZATION (node->decl);
  if (!opts)
    opts = optimization_default_node;
  return (opts == proof_optimization
	  && !lookup_attribute ("optimize", DECL_ATTRIBUTES (node->decl)));
}

/* In the current function, a shadow or a copy, make each direct call of a
   function that has a copy in FIRST, or else in SECOND (each keyed by the
   function's ultimate alias target; either may be null), a call of the
   copy.  */

static void
redirect_to_copies (hash_map<cgraph_node *, cgraph_node *> *first,
		    hash_map<cgraph_node *, cgraph_node *> *second = NULL)
{
  bool changed = false;
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	 gsi_next (&gsi))
      if (gcall *call = dyn_cast <gcall *> (gsi_stmt (gsi)))
	if (tree fn = gimple_call_fndecl (call))
	  if (cgraph_node *n = cgraph_node::get (fn))
	    {
	      cgraph_node *target = n->ultimate_alias_target ();
	      cgraph_node **c = first ? first->get (target) : NULL;
	      if (!c && second)
		c = second->get (target);
	      if (c)
		{
		  gimple_call_set_fndecl (call, (*c)->decl);
		  update_stmt (call);
		  changed = true;
		}
	    }
  if (changed)
    cgraph_edge::rebuild_edges ();
}

/* The functions a shadow of one of ROOTS may reach through direct calls,
   transitively, in the order found.  */

static void
reached_from (const vec<cgraph_node *> &roots, vec<cgraph_node *> &reached)
{
  hash_set<cgraph_node *> seen;
  for (cgraph_node *node : roots)
    for (cgraph_edge *e = node->callees; e; e = e->next_callee)
      {
	cgraph_node *callee = e->callee->ultimate_alias_target ();
	if (callee->has_gimple_body_p () && !seen.add (callee))
	  reached.safe_push (callee);
      }
  for (unsigned i = 0; i < reached.length (); ++i)
    for (cgraph_edge *e = reached[i]->callees; e; e = e->next_callee)
      {
	cgraph_node *callee = e->callee->ultimate_alias_target ();
	if (callee->has_gimple_body_p () && !seen.add (callee))
	  reached.safe_push (callee);
      }
}

/* D4405's proof optimization level.  TODO lists the functions holding a
   region.  The shadows are optimized at the proof level
   (set_proof_options), but the inliner does not inline a function
   optimized at another level into them after the early optimizations (a
   function at -O1 or -Og into a shadow at level 2, say).  So make a level
   copy of each function a shadow may reach -- through the direct calls of
   a function holding an obligation, and of what it calls, transitively --
   that is not at the proof level or calls one that has a level copy, if
   it can be copied and the program runs its body or one like it (it is
   not interposable): a version of its body at the proof level, folded as
   the real function is, whose calls of such functions are calls of their
   copies, recorded in COPIES.  None is needed where every such function
   is at the proof level, as it is by default at -O2 and -O3.  A copy is
   local, is not forced, and is analysis-only code (analysis_only_fn_p);
   the shadows call it, and what they do not inline is gone with them.  */

static void
make_level_copies (const vec<cgraph_node *> &todo,
		   hash_map<cgraph_node *, cgraph_node *> &copies)
{
  if (!proof_optimization)
    return;
  auto_vec<cgraph_node *> roots;
  for (cgraph_node *node : todo)
    {
      push_cfun (DECL_STRUCT_FUNCTION (node->decl));
      bool root = false;
      basic_block bb;
      FOR_EACH_BB_FN (bb, cfun)
	for (gimple_stmt_iterator gsi = gsi_start_bb (bb);
	     !root && !gsi_end_p (gsi); gsi_next (&gsi))
	  root = gimple_call_internal_p (gsi_stmt (gsi),
					 IFN_CONTRACT_UNPROVEN);
      if (root && shadowable_p (node))
	roots.safe_push (node);
      pop_cfun ();
    }
  auto_vec<cgraph_node *> reached;
  reached_from (roots, reached);

  hash_set<cgraph_node *> copyable, copied;
  for (cgraph_node *node : reached)
    {
      push_cfun (DECL_STRUCT_FUNCTION (node->decl));
      /* As for inlining, a refinable body counts as available
	 (cgraph_node::refinable_p): a call of a copy made out of line then
	 makes the proof conditional (out_of_line_premises_p).  */
      if (shadowable_p (node)
	  && node->get_availability (NULL, true) >= AVAIL_AVAILABLE)
	copyable.add (node);
      pop_cfun ();
    }
  for (bool changed = true; changed; )
    {
      changed = false;
      for (cgraph_node *node : reached)
	if (copyable.contains (node) && !copied.contains (node))
	  {
	    bool needed = !at_proof_level_p (node);
	    for (cgraph_edge *e = node->callees; e && !needed;
		 e = e->next_callee)
	      needed = copied.contains (e->callee->ultimate_alias_target ());
	    if (needed)
	      {
		copied.add (node);
		changed = true;
	      }
	  }
    }

  auto_vec<cgraph_node *> made;
  for (cgraph_node *node : reached)
    if (copied.contains (node))
      {
	char suffix[32];
	sprintf (suffix, "contract_level_%u", level_copies);
	cgraph_node *copy = analysis_version (node, suffix);
	if (!copy)
	  continue;
	++level_copies;
	DECL_ATTRIBUTES (copy->decl)
	  = shadow_attributes (DECL_ATTRIBUTES (node->decl), 0, level_attr);
	DECL_IGNORED_P (copy->decl) = 1;
	set_proof_options (copy->decl);
	copies.put (node, copy);
	made.safe_push (copy);
	if (dump_file)
	  fprintf (dump_file, "Level copy %s of %s\n", copy->dump_name (),
		   node->dump_name ());
      }
  auto_vec<unsigned> none;
  for (cgraph_node *copy : made)
    {
      push_cfun (DECL_STRUCT_FUNCTION (copy->decl));
      fold_regions (NULL, none, LEVEL_COPY);
      redirect_to_copies (&copies);
      pop_cfun ();
    }
}

/* qsort comparison of two region kinds.  */

static int
compare_kinds (const void *a, const void *b)
{
  unsigned x = *(const unsigned *) a;
  unsigned y = *(const unsigned *) b;
  return x < y ? -1 : x > y ? 1 : 0;
}

/* Make the shadows of NODE, which holds a region -- one per kind among its
   obligations: the ordinary shadow for ordinary ones, an override shadow
   per override pair -- or remove its regions when it cannot be shadowed or
   holds no obligation.  If NODE calls a function with a premise copy, in
   COPIES (make_premise_copies), the ordinary shadow gets a premise shadow
   too: another copy of the body, whose calls of such functions are calls
   of their copies.  Every shadow is optimized at the proof level, and its
   calls of a function with a level copy, in LEVEL (make_level_copies), are
   calls of the copy -- in a premise shadow, of one without a premise
   copy.  */

static void
process_function (cgraph_node *node,
		  hash_map<cgraph_node *, cgraph_node *> *copies,
		  hash_map<cgraph_node *, cgraph_node *> *level)
{
  tree decl = node->decl;
  push_cfun (DECL_STRUCT_FUNCTION (decl));
  auto_vec<unsigned> kinds;
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	 gsi_next (&gsi))
      if (gimple_call_internal_p (gsi_stmt (gsi), IFN_CONTRACT_UNPROVEN))
	{
	  unsigned id = tree_to_uhwi (gimple_call_arg (gsi_stmt (gsi), 0));
	  gcc_checking_assert (id < obligations.length ());
	  if (!kinds.contains (obligations[id].pair))
	    kinds.safe_push (obligations[id].pair);
	}
  bool has_obligation = !kinds.is_empty ();
  bool shadow_p = has_obligation && shadowable_p (node);
  pop_cfun ();

  /* In order of kind: the ordinary shadow first.  */
  kinds.qsort (compare_kinds);
  auto_vec<cgraph_node *> made;
  auto_vec<unsigned> shadowed;
  if (shadow_p)
    for (unsigned kind : kinds)
      {
	unsigned index = shadows.length ();
	char suffix[32];
	sprintf (suffix, "%s_%u",
		 kind ? "contract_override" : "contract_shadow", index);
	cgraph_node *shadow = analysis_version (node, suffix);
	if (!shadow)
	  continue;
	tree sdecl = shadow->decl;
	DECL_ATTRIBUTES (sdecl)
	  = shadow_attributes (DECL_ATTRIBUTES (decl), index);
	set_proof_options (sdecl);
	DECL_UNINLINABLE (sdecl) = 1;
	DECL_DISREGARD_INLINE_LIMITS (sdecl) = 0;
	DECL_IGNORED_P (sdecl) = 1;
	/* Kept by nothing but this: like any forced function, it is not
	   local (IPA assumes a local function without callers is gone).  */
	shadow->force_output = 1;
	shadow->local = 0;
	shadow_info *info = new shadow_info ();
	info->pair = kind;
	shadows.safe_push (info);
	vec_safe_push (shadow_real_fns, decl);
	++live_shadows;
	made.safe_push (shadow);
	shadowed.safe_push (kind);
      }

  /* The premise shadow, if the ordinary shadow was made and NODE calls a
     function with a premise copy.  */
  cgraph_node *premise_shadow = NULL;
  if (copies && !made.is_empty () && shadowed[0] == 0)
    for (cgraph_edge *e = node->callees; e; e = e->next_callee)
      if (copies->get (e->callee->ultimate_alias_target ()))
	{
	  unsigned index = shadows.length ();
	  char suffix[40];
	  sprintf (suffix, "contract_premise_shadow_%u", index);
	  premise_shadow = analysis_version (node, suffix);
	  if (!premise_shadow)
	    break;
	  tree sdecl = premise_shadow->decl;
	  DECL_ATTRIBUTES (sdecl)
	    = shadow_attributes (DECL_ATTRIBUTES (decl), index);
	  set_proof_options (sdecl);
	  DECL_UNINLINABLE (sdecl) = 1;
	  DECL_DISREGARD_INLINE_LIMITS (sdecl) = 0;
	  DECL_IGNORED_P (sdecl) = 1;
	  premise_shadow->force_output = 1;
	  premise_shadow->local = 0;
	  unsigned ordinary = shadow_index_of (made[0]->decl);
	  shadow_info *info = new shadow_info ();
	  info->premise_of = ordinary + 1;
	  shadows.safe_push (info);
	  vec_safe_push (shadow_real_fns, decl);
	  ++live_shadows;
	  break;
	}

  if (dump_file)
    {
      for (unsigned i = 0; i < made.length (); ++i)
	if (shadowed[i])
	  fprintf (dump_file, "Override shadow %s of %s for pair %u\n",
		   made[i]->dump_name (), node->dump_name (), shadowed[i]);
	else
	  fprintf (dump_file, "Shadow %s of %s\n", made[i]->dump_name (),
		   node->dump_name ());
      if (premise_shadow)
	fprintf (dump_file, "Premise shadow %s of %s\n",
		 premise_shadow->dump_name (), node->dump_name ());
      if (has_obligation && made.length () < kinds.length ())
	fprintf (dump_file, "No shadow for %s\n", node->dump_name ());
      else if (!has_obligation)
	fprintf (dump_file, "No obligation in %s\n", node->dump_name ());
    }

  /* Every shadow is a copy of the body with its regions; only now are they
     folded.  The real function: ignore's code, but for the inert
     assumptions of -fcontracts-prove-assume, whose guards contract_proven
     folds once the shadow has decided the obligations.  */
  push_cfun (DECL_STRUCT_FUNCTION (decl));
  fold_regions (NULL, shadowed);
  pop_cfun ();

  for (cgraph_node *shadow : made)
    {
      push_cfun (DECL_STRUCT_FUNCTION (shadow->decl));
      fold_regions (shadow_info_of (shadow->decl), shadowed);
      if (level)
	redirect_to_copies (level);
      pop_cfun ();
    }
  if (premise_shadow)
    {
      push_cfun (DECL_STRUCT_FUNCTION (premise_shadow->decl));
      fold_regions (shadow_info_of (premise_shadow->decl), shadowed);
      redirect_to_copies (copies, level);
      pop_cfun ();
    }
}

/* -fcontracts-prove-callee-premises.  TODO lists the functions holding a
   region.  Find the functions that hold an ordinary assumption region of an
   evaluation that is not guaranteed, which a shadow may reach -- through
   the direct calls of a function holding an ordinary obligation, and of
   what it calls, transitively -- and those on the way to them, and make a
   premise copy of each, recorded in COPIES: a version of its body in which
   only those regions and the guaranteed ones are executed, assumed as in a
   shadow, which holds no obligation, and whose calls of such functions are
   calls of their copies, and of other functions calls of their level
   copies, in LEVEL (make_level_copies).  A copy is optimized at the proof
   level, is local, is not forced, and is analysis-only code
   (analysis_only_fn_p); the premise shadows call it, and what they do not
   inline is gone with them.  */

static void
make_premise_copies (const vec<cgraph_node *> &todo,
		     hash_map<cgraph_node *, cgraph_node *> &copies,
		     hash_map<cgraph_node *, cgraph_node *> &level)
{
  /* What each function holding a region holds.  */
  hash_set<cgraph_node *> premise, roots;
  for (cgraph_node *node : todo)
    {
      push_cfun (DECL_STRUCT_FUNCTION (node->decl));
      basic_block bb;
      FOR_EACH_BB_FN (bb, cfun)
	for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	     gsi_next (&gsi))
	  {
	    gimple *stmt = gsi_stmt (gsi);
	    if (gimple_call_internal_p (stmt, IFN_CONTRACT_ASSUMPTION)
		&& integer_zerop (gimple_call_arg (stmt, 0))
		&& region_kind (stmt, 1) == 0)
	      premise.add (node);
	    else if (gimple_call_internal_p (stmt, IFN_CONTRACT_UNPROVEN)
		     && (obligations[tree_to_uhwi (gimple_call_arg (stmt, 0))]
			 .pair == 0))
	      roots.add (node);
	  }
      pop_cfun ();
    }
  if (premise.is_empty () || roots.is_empty ())
    return;

  /* What a shadow may reach, in the order found.  */
  auto_vec<cgraph_node *> root_list;
  for (cgraph_node *node : todo)
    if (roots.contains (node))
      root_list.safe_push (node);
  auto_vec<cgraph_node *> reached;
  reached_from (root_list, reached);

  /* Those that hold such a region or call one that is copied, if they can
     be copied.  */
  hash_set<cgraph_node *> copied;
  hash_set<cgraph_node *> copyable;
  for (cgraph_node *node : reached)
    {
      push_cfun (DECL_STRUCT_FUNCTION (node->decl));
      if (shadowable_p (node))
	copyable.add (node);
      pop_cfun ();
    }
  for (bool changed = true; changed; )
    {
      changed = false;
      for (cgraph_node *node : reached)
	if (copyable.contains (node) && !copied.contains (node))
	  {
	    bool needed = premise.contains (node);
	    for (cgraph_edge *e = node->callees; e && !needed;
		 e = e->next_callee)
	      needed = copied.contains (e->callee->ultimate_alias_target ());
	    if (needed)
	      {
		copied.add (node);
		changed = true;
	      }
	  }
    }

  auto_vec<cgraph_node *> made;
  for (cgraph_node *node : reached)
    if (copied.contains (node))
      {
	char suffix[32];
	sprintf (suffix, "contract_premise_%u", premise_copies);
	cgraph_node *copy = analysis_version (node, suffix);
	if (!copy)
	  continue;
	++premise_copies;
	DECL_ATTRIBUTES (copy->decl)
	  = shadow_attributes (DECL_ATTRIBUTES (node->decl), 0, premise_attr);
	set_proof_options (copy->decl);
	DECL_IGNORED_P (copy->decl) = 1;
	copies.put (node, copy);
	made.safe_push (copy);
	if (dump_file)
	  fprintf (dump_file, "Premise copy %s of %s\n", copy->dump_name (),
		   node->dump_name ());
      }
  auto_vec<unsigned> none;
  for (cgraph_node *copy : made)
    {
      push_cfun (DECL_STRUCT_FUNCTION (copy->decl));
      fold_regions (NULL, none, PREMISE_COPY);
      redirect_to_copies (&copies, &level);
      pop_cfun ();
    }
}

/* True if, in the code NODE stands for -- following the calls the inliner
   decided to inline into it -- a call still made out of line is at one of
   SITES: a call of the same function at the same site, of any copy of it
   at a site that is not the last of its chain (what that copy inlined
   below is not followed), and of a copy another translation unit may
   provide at the last; or an indirect call at such a site.  When ROOT is
   non-null, only the calls within that block count.  */

static bool
out_of_line_site_p (cgraph_node *node, const vec<inline_site> &sites,
		    tree root)
{
  for (cgraph_edge *e = node->callees; e; e = e->next_callee)
    {
      if (!e->inline_failed)
	{
	  if (out_of_line_site_p (e->callee, sites, NULL_TREE))
	    return true;
	  continue;
	}
      if (!e->call_stmt || (root && !within_block_p (e->call_stmt, root)))
	continue;
      cgraph_node *target = e->callee->ultimate_alias_target ();
      int uid = DECL_UID (node_origin (target));
      location_t locus = call_locus (e->call_stmt, e->callee->decl);
      for (const inline_site &site : sites)
	if (site.fn_uid == uid && site.locus == locus
	    && (!site.last || replaceable_p (target->decl)))
	  return true;
    }
  for (cgraph_edge *e = node->indirect_calls; e; e = e->next_callee)
    {
      if (!e->call_stmt || (root && !within_block_p (e->call_stmt, root)))
	continue;
      location_t locus = call_locus (e->call_stmt, NULL_TREE);
      for (const inline_site &site : sites)
	if (site.locus == locus)
	  return true;
    }
  return false;
}

/* True if NODE or a clone of it -- each a copy of the same body, an inline
   clone the inliner decided on or an IPA clone -- makes a call at one of
   SITES out of line (out_of_line_site_p).  */

static bool
copies_out_of_line_p (cgraph_node *node, const vec<inline_site> &sites)
{
  if (out_of_line_site_p (node, sites, NULL_TREE))
    return true;
  for (cgraph_node *c = node->clones; c; c = c->next_sibling_clone)
    if (copies_out_of_line_p (c, sites))
      return true;
  return false;
}

/* True if the code of NODE, with what the inliner decided to inline into
   it, holds an evaluation noted by contract_shadow_note_premise.  */

static bool
inline_tree_premise_p (cgraph_node *node)
{
  if (premise_fn_p (node_origin (node)))
    return true;
  for (cgraph_edge *e = node->callees; e; e = e->next_callee)
    if (!e->inline_failed && inline_tree_premise_p (e->callee))
      return true;
  return false;
}

static cgraph_node *callees_out_of_line_premises_p (cgraph_node *);

/* True if a call made out of line to NODE may let what IPA knows of it --
   the summaries a shadow uses: that it is noreturn, const, pure, its memory
   effects -- depend on evaluations noted by contract_shadow_note_premise
   that the definition the program calls need not make: NODE is (a copy of)
   a function another translation unit may provide whose code holds such
   evaluations, or it calls out of line a function for which this is
   true.  */

static bool
out_of_line_premises_p (cgraph_node *node)
{
  /* A function without a body here (any more: a C inline definition or an
     extern inline function that was not inlined) counts too, if its body
     held such evaluations: IPA may have analyzed it before.  */
  node = node->ultimate_alias_target ();
  int uid = node->get_uid ();
  if (!premise_memo)
    premise_memo = new hash_map<int_hash<int, -1, -2>, bool>;
  if (bool *known = premise_memo->get (uid))
    return *known;
  /* False while being computed: a cycle adds nothing.  */
  premise_memo->put (uid, false);
  /* A split part (ipa-split) is called only from its function's head,
     which is this TU's code where it was inlined (the inlining itself is
     what contract_shadow_note_inline records), so the part's code is
     this TU's wherever it runs.  A refinable body is interposable for the
     calls that do not inline it, so IPA tells them nothing of its code,
     whatever its evaluations -- but a local clone of one is available,
     and the real function may call the function itself.  */
  bool r = ((!node->split_part && replaceable_origin_p (node->decl)
	     && (inline_tree_premise_p (node)
		 || (!replaceable_p (node->decl)
		     && refinable_body_p (node->decl))))
	    || callees_out_of_line_premises_p (node) != NULL);
  premise_memo->put (uid, r);
  return r;
}

/* The function that the code of NODE, with what the inliner decided to
   inline into it, calls out of line and for which out_of_line_premises_p
   is true, or NULL if none.  */

static cgraph_node *
callees_out_of_line_premises_p (cgraph_node *node)
{
  for (cgraph_edge *e = node->callees; e; e = e->next_callee)
    if (!e->inline_failed)
      {
	if (cgraph_node *found = callees_out_of_line_premises_p (e->callee))
	  return found;
      }
    else if (out_of_line_premises_p (e->callee))
      return e->callee;
  return NULL;
}

/* True if the current function, shadow INFO of the function REAL, may rely
   on a premise taken from a contract evaluation that this translation unit
   does not decide (see premise_fns), and say why in the dump.  */

static bool
undecided_premise_p (shadow_info *info, tree real)
{
  /* A level copy of a refinable body is local, so IPA tells a call of it
     what its code does.  */
  if (!premise_fns && !level_copies)
    return false;
  cgraph_node *node = cgraph_node::get (current_function_decl);
  if (cgraph_node *callee = node ? callees_out_of_line_premises_p (node) : NULL)
    {
      if (dump_file)
	fprintf (dump_file, "Calls %s out of line, whose summary may rest "
		 "on contract evaluations another definition need not "
		 "make\n", callee->dump_name ());
      return true;
    }
  if (info->sites.is_empty ())
    return false;
  cgraph_node *real_node = cgraph_node::get (real);
  if (real_node && copies_out_of_line_p (real_node, info->sites))
    {
      if (dump_file)
	fprintf (dump_file, "Inlined code holding contract evaluations "
		 "another definition need not make, which %s calls out of "
		 "line\n", real_node->dump_name ());
      return true;
    }
  return false;
}

/* Find out, for shadow FNDECL, the current function, whether it relies on
   a premise this TU does not decide, while the call graph edges of the
   shadow and of the real function exist: when the shadow's late
   optimizations begin (contract_proven), or when it is decided after the
   early optimizations (-flto).  */

static void
note_undecided_premise (tree fndecl)
{
  unsigned index = shadow_index_of (fndecl);
  shadow_info *info = shadows[index];
  if (info->undecided_known)
    return;
  info->undecided_known = true;
  info->undecided = undecided_premise_p (info, (*shadow_real_fns)[index]);
}

/* Put the ordinary shadows in TODO, a list of shadows to decide, ahead of
   the premise shadows, which retry what they leave unproven
   (settle_retry), and those ahead of the override shadows, which may take
   the ordinary obligations as premises (decide_shadow), keeping the order
   otherwise.  */

static void
order_shadows (vec<cgraph_node *> &todo)
{
  auto_vec<cgraph_node *> retries, overrides;
  unsigned n = 0;
  for (cgraph_node *node : todo)
    {
      shadow_info *info = shadow_info_of (node->decl);
      if (info->pair)
	overrides.safe_push (node);
      else if (info->premise_of)
	retries.safe_push (node);
      else
	todo[n++] = node;
    }
  todo.truncate (n);
  for (cgraph_node *node : retries)
    todo.quick_push (node);
  for (cgraph_node *node : overrides)
    todo.quick_push (node);
}

/* Recompute the result of obligation ID from what the shadows decided
   (shadow_decisions).  */

static void
remerge_result (unsigned id)
{
  obligation &o = obligations[id];
  o.result = CSR_PENDING;
  for (const shadow_decision &d : shadow_decisions)
    if (d.id == id)
      merge_result (o, d.result);
}

/* True if the premise shadow INFO is worth deciding: the ordinary shadow it
   retries left an obligation unproven.  */

static bool
retry_needed_p (const shadow_info *info)
{
  for (const shadow_decision &d : shadow_decisions)
    if (d.shadow == info->premise_of - 1
	&& (d.result == CSR_UNPROVEN || d.result == CSR_ALWAYS_FALSE))
      return true;
  return false;
}

/* -fcontracts-prove-callee-premises: the premise shadow of the ordinary
   shadow ORDINARY, whose own decisions stand as made, proved PROVEN: each
   obligation ORDINARY left unproven or always false that is in PROVEN is
   proven conditionally -- the copies' premises are not guaranteed -- so it
   is not diagnosed and never assumed.  A sound proof is never changed.  */

static void
settle_retry (unsigned ordinary,
	      hash_set<int_hash<unsigned, -1u, -2u> > &proven)
{
  for (shadow_decision &d : shadow_decisions)
    if (d.shadow == ordinary
	&& (d.result == CSR_UNPROVEN || d.result == CSR_ALWAYS_FALSE)
	&& proven.contains (d.id))
      {
	d.result = CSR_CONDITIONAL;
	remerge_result (d.id);
	if (dump_file)
	  fprintf (dump_file, "callee premises: obligation %u (line %d): "
		   "proven, conditional\n", d.id,
		   expand_location (obligations[d.id].loc).line);
      }
}

/* A premise shadow, NODE, that is not worth deciding (retry_needed_p):
   discard it without optimizing it further.  */

static void
skip_retry (cgraph_node *node)
{
  gcc_checking_assert (live_shadows);
  --live_shadows;
  node->process = 0;
  node->remove_symbol_and_inline_clones ();
}

/* True if block B ends in the condition on a nested obligation's guard
   (CONTRACT_SHADOW_NESTED_KIND).  */

static bool
nested_branch_p (basic_block b)
{
  gcond *c = safe_dyn_cast <gcond *> (*gsi_last_bb (b));
  if (!c || TREE_CODE (gimple_cond_lhs (c)) != SSA_NAME)
    return false;
  gimple *def = SSA_NAME_DEF_STMT (gimple_cond_lhs (c));
  return (gimple_call_internal_p (def, IFN_CONTRACT_SHADOW_GUARD)
	  && region_kind (def, 0) == CONTRACT_SHADOW_NESTED_KIND);
}

/* True if TARGET is reached from block B, in the current function, through
   blocks with a single successor each, or ending in the condition on a
   nested obligation's guard, both of whose edges are followed (to DEPTH
   such conditions).  */

static bool
reaches_unconditionally_p (basic_block b, basic_block target, int depth)
{
  for (int n = n_basic_blocks_for_fn (cfun); n > 0; --n)
    {
      if (depth < 16 && nested_branch_p (b))
	{
	  edge e;
	  edge_iterator ei;
	  FOR_EACH_EDGE (e, ei, b->succs)
	    if (!(e->flags & (EDGE_ABNORMAL | EDGE_EH))
		&& (e->dest == target
		    || reaches_unconditionally_p (e->dest, target,
						  depth + 1)))
	      return true;
	  return false;
	}
      if (!single_succ_p (b)
	  || (single_succ_edge (b)->flags & (EDGE_ABNORMAL | EDGE_EH)))
	return false;
      b = single_succ (b);
      if (b == target)
	return true;
      if (b == EXIT_BLOCK_PTR_FOR_FN (cfun))
	return false;
    }
  return false;
}

/* Decide the obligations in the current function, a shadow, after its
   optimization, and dump what was decided.  */

static void
decide_shadow ()
{
  shadow_info *info = shadow_info_of (current_function_decl);
  hash_set<int_hash<unsigned, -1u, -2u> > unproven, always_false;

  /* A marker reached from the entry through blocks with a single
     successor each happens whenever the function is entered; nothing
     guarded it, so the predicate is false whenever it is evaluated.  The
     side branch holding a nested obligation's region is no condition
     (CONTRACT_SHADOW_NESTED_KIND): both its edges are followed.  */
  auto unconditional_p = [] (basic_block target) {
    return reaches_unconditionally_p (ENTRY_BLOCK_PTR_FOR_FN (cfun), target,
				      0);
  };

  /* A last try with the ranger at a marker whose block is entered only from
     a condition: one the last value-range pass left can still be known.  */
  bool ranger_p = gimple_in_ssa_p (cfun);
  if (ranger_p)
    enable_ranger (cfun);
  auto excluded_p = [ranger_p] (basic_block bb) {
    if (!ranger_p || !single_pred_p (bb))
      return false;
    edge e = single_pred_edge (bb);
    gcond *c = safe_dyn_cast <gcond *> (*gsi_last_bb (e->src));
    if (!c)
      return false;
    int_range_max r;
    if (!get_range_query (cfun)->range_of_stmt (r, c)
	|| r.undefined_p () || !r.singleton_p ())
      return false;
    bool taken = !r.zero_p ();
    return ((e->flags & EDGE_TRUE_VALUE) && !taken)
	   || ((e->flags & EDGE_FALSE_VALUE) && taken);
  };

  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	 gsi_next (&gsi))
      if (gimple_call_internal_p (gsi_stmt (gsi), IFN_CONTRACT_UNPROVEN))
	{
	  unsigned id = tree_to_uhwi (gimple_call_arg (gsi_stmt (gsi), 0));
	  if (excluded_p (bb))
	    continue;
	  unproven.add (id);
	  if (unconditional_p (bb))
	    always_false.add (id);
	}
  if (ranger_p)
    disable_ranger (cfun);

  /* A premise shadow only says which of the obligations its ordinary
     shadow left unproven it proves (settle_retry).  */
  if (info->premise_of)
    {
      hash_set<int_hash<unsigned, -1u, -2u> > proven;
      for (unsigned id : info->ids)
	if (!unproven.contains (id))
	  proven.add (id);
      if (dump_file)
	for (unsigned id : info->ids)
	  fprintf (dump_file, "callee premises: obligation %u (line %d): "
		   "%s\n", id, expand_location (obligations[id].loc).line,
		   proven.contains (id) ? "proven" : "not proven");
      settle_retry (info->premise_of - 1, proven);
      gcc_checking_assert (live_shadows);
      --live_shadows;
      return;
    }

  /* What the shadow relies on that may not hold in the program.  */
  bool premises = info->conditional;
  for (unsigned id : info->ids)
    if (side_effects_p (obligations[id]))
      premises = true;
  for (unsigned dep : info->assumption_deps)
    if (assumption_side_effects_p (dep))
      {
	premises = true;
	if (dump_file)
	  fprintf (dump_file, "Assumes a predicate calling a function that "
		   "may have side effects\n");
	break;
      }
  if (!info->undecided_known)
    note_undecided_premise (current_function_decl);
  if (info->undecided)
    {
      premises = true;
      if (dump_file)
	fprintf (dump_file, "Relies on a premise this translation unit does "
		 "not decide\n");
    }
  /* An override shadow took the ordinary obligations whose markers it met
     as premises.  The ordinary shadows are decided first
     (contract_shadow_expand_first), so their results are known: a proof
     relying on one not proven soundly is conditional.  */
  for (unsigned id : info->relied)
    if (effective_result (id) != CSR_SOUND)
      {
	premises = true;
	if (dump_file)
	  fprintf (dump_file, "Relies on contract obligation %u, which is "
		   "%s\n", id, result_name (effective_result (id)));
	break;
      }
  /* A proof is conditional on the predicates of the markers that survive
     ahead of it, which a check would have failed.  Not on a nested
     obligation's (DECISIONS.md E169 (12)): its region is a side branch
     that never joins the path again (CONTRACT_SHADOW_NESTED_KIND), so
     what follows it takes nothing from it.  */
  bool conditional = premises;
  for (unsigned id : info->ids)
    if (unproven.contains (id) && !obligations[id].parent)
      conditional = true;
  for (unsigned id : info->ids)
    {
      contract_shadow_result r;
      /* E195: a predicate folding found false wherever it is evaluated is
	 always false wherever the marker survives.  */
      if (unproven.contains (id))
	r = (obligations[id].folded_false
	     || (always_false.contains (id) && !premises
		 && !info->in_loop.contains (id))
	     ? CSR_ALWAYS_FALSE : CSR_UNPROVEN);
      else
	r = conditional ? CSR_CONDITIONAL : CSR_SOUND;
      merge_result (obligations[id], r);
      if (premise_copies)
	{
	  shadow_decision d = { id, shadow_index_of (current_function_decl),
				r };
	  shadow_decisions.safe_push (d);
	}
      /* Each copy of the real function must inline what the proof relied
	 on for an assumption made of it to stay (check_assumption_copies):
	 one may have been inlined where this shadow could not see.  */
      if (r == CSR_SOUND)
	for (const inline_site &site : info->sites)
	  {
	    obligation_site os = { id, site };
	    obligation_sites.safe_push (os);
	  }
      if (dump_file && obligations[id].named)
	{
	  /* All of a pair's obligations are at the overriding function's
	     definition; the predicate tells them apart.  */
	  const obligation &o = obligations[id];
	  expanded_location xloc
	    = expand_location (o.pred_loc != UNKNOWN_LOCATION ? o.pred_loc
			       : o.loc);
	  if (info->pair)
	    fprintf (dump_file, "contract obligation %u (line %d, override "
		     "pair %u): %s\n", id, xloc.line, info->pair,
		     result_name (r));
	  else
	    fprintf (dump_file, "contract obligation %u (line %d, override "
		     "pair %u in the ordinary shadow): %s\n", id, xloc.line,
		     o.named, result_name (r));
	}
      else if (dump_file)
	{
	  expanded_location xloc = expand_location (obligations[id].loc);
	  fprintf (dump_file, "contract obligation %u (line %d): %s\n", id,
		   xloc.line, result_name (r));
	}
    }
  /* E169: what the obligations nested in each come to, as far as decided
     now (other copies of the function may not be).  */
  if (dump_file)
    for (unsigned id : info->ids)
      {
	int line = expand_location (obligations[id].loc).line;
	if (obligations[id].parent)
	  fprintf (dump_file, "contract obligation %u (line %d): nested in "
		   "%u\n", id, line, obligations[id].parent - 1);
	if (has_nested_p (id))
	  fprintf (dump_file, "contract obligation %u (line %d), with those "
		   "nested in it: %s\n", id, line,
		   result_name (effective_result (id)));
      }
  gcc_checking_assert (live_shadows);
  --live_shadows;
}

/* Fold every .CONTRACT_PROVEN guard of the current function: to true if
   its obligation was proven soundly, to false otherwise.  Returns true if
   it held any.  */

static bool
fold_proven_guards ()
{
  hash_map<tree, bool> guards;
  bool any = false;
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	 gsi_next (&gsi))
      if (gimple_call_internal_p (gsi_stmt (gsi), IFN_CONTRACT_PROVEN))
	{
	  unsigned id = tree_to_uhwi (gimple_call_arg (gsi_stmt (gsi), 0));
	  bool sound = (id < obligations.length ()
			&& effective_result (id) == CSR_SOUND);
	  if (dump_file)
	    fprintf (dump_file, "contract obligation %u: %s\n", id,
		     sound ? "assumed" : "not assumed");
	  replace_guard (&gsi, sound, guards);
	  any = true;
	}
  if (!guards.is_empty ())
    fold_guard_conditions (guards);
  return any;
}

/* The outermost block of the copy, inlined into the current function, of
   the function with DECL_UID FN_UID that holds STMT; NULL_TREE if STMT is
   that function's own (or no such block is found: then the whole function
   is searched).  */

static tree
copy_root (gimple *stmt, int fn_uid)
{
  for (tree b = gimple_block (stmt); b && TREE_CODE (b) == BLOCK;
       b = BLOCK_SUPERCONTEXT (b))
    if (inlined_fn_uid (b) == fn_uid)
      return b;
  return NULL_TREE;
}

/* The obligation of which FN is the assumption function, or a clone of it
   (IPA passes now clone and rewrite assumption functions, ipa-sra dropping
   unused parameters); NULL if none.  */

static unsigned *
assumption_fn_id (tree fn)
{
  if (unsigned *id = assumption_fns->get (DECL_UID (fn)))
    return id;
  cgraph_node *node = cgraph_node::get (fn);
  if (!node)
    return NULL;
  while (node->clone_of)
    node = node->clone_of;
  if (node->decl != fn)
    return assumption_fns->get (DECL_UID (node->decl));
  if (node->former_clone_of)
    return assumption_fns->get (DECL_UID (node->former_clone_of));
  return NULL;
}

/* The obligation whose assumption the .ASSUME call STMT is, or -1 if it is
   no -fcontracts-prove-assume assumption.  */

static int
assumption_id (gimple *stmt)
{
  if (!assumption_fns || !gimple_call_internal_p (stmt, IFN_ASSUME)
      || gimple_call_num_args (stmt) == 0)
    return -1;
  tree a = gimple_call_arg (stmt, 0);
  if (TREE_CODE (a) != ADDR_EXPR
      || TREE_CODE (TREE_OPERAND (a, 0)) != FUNCTION_DECL)
    return -1;
  unsigned *id = assumption_fn_id (TREE_OPERAND (a, 0));
  return id ? (int) *id : -1;
}

/* In the current function NODE, after the inliner's decisions are applied,
   remove each assumption made of a sound proof (-fcontracts-prove-assume)
   whose proof relied on a call being inlined that this copy of the
   function holding the obligation makes out of line: the definition it
   calls need not make the evaluations the shadow took premises from.  If
   NODE keeps any such assumption, mark it as a refinable body: a proof is
   this translation unit's, so another's definition of the function may
   assume nothing.  Also remove each assumption made of an obligation not
   proven soundly.  Returns true if anything was removed: the caller then
   updates which variables have their address taken.  */

static bool
check_assumption_copies (cgraph_node *node)
{
  if (!assumption_fns || !node || contract_shadow_function_p (node->decl))
    return false;
  bool removed = false, keeps = false;
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi); )
      {
	gimple *stmt = gsi_stmt (gsi);
	int id = assumption_id (stmt);
	if (id < 0)
	  {
	    gsi_next (&gsi);
	    continue;
	  }
	/* An assumption made of an obligation not proven soundly is vacuous
	   (its guard folds to false).  Remove it now rather than at
	   expansion: the call's arguments -- the address of a variable its
	   predicate reads through a reference, as an outlined check's
	   does -- would otherwise keep that variable in memory through the
	   late optimizations, and the code would differ from what it is
	   with no analysis at all.  */
	if ((unsigned) id >= obligations.length ()
	    || effective_result (id) != CSR_SOUND)
	  {
	    if (dump_file)
	      fprintf (dump_file, "contract obligation %d: not proven "
		       "soundly, its assumption removed\n", id);
	    node->remove_stmt_references (stmt);
	    unlink_stmt_vdef (stmt);
	    gsi_remove (&gsi, true);
	    removed = true;
	    continue;
	  }
	auto_vec<inline_site> sites;
	for (const obligation_site &os : obligation_sites)
	  if (os.id == (unsigned) id)
	    sites.safe_push (os.site);
	if (!sites.is_empty ()
	    && out_of_line_site_p (node, sites,
				   copy_root (stmt, obligations[id].fn_uid)))
	  {
	    if (dump_file)
	      fprintf (dump_file, "contract obligation %d: not assumed in "
		       "this copy, which calls out of line what its proof "
		       "inlined\n", id);
	    node->remove_stmt_references (stmt);
	    unlink_stmt_vdef (stmt);
	    gsi_remove (&gsi, true);
	    removed = true;
	    continue;
	  }
	keeps = true;
	gsi_next (&gsi);
      }
  /* The function is usually marked already, by the front end, for the
     prove or diagnose evaluation itself; not for a caller-side one, which
     is deferred to the shadow at the call rather than evaluated in a
     contract wrapper.  */
  if (keeps)
    mark_refinable_body (node->decl);
  return removed;
}

/* Note, after the diagnostic of obligation ID, each nested obligation with
   prove that made it unproven, and each note of a diagnostic decided at
   once.  The texts are those of c-family/contracts-prove.cc's
   inform_cause.  */

static void
inform_nested (unsigned id)
{
  auto note = [] (location_t loc, bool always_false) {
    if (always_false)
      inform (loc, "precondition of a call in the predicate is always "
	      "false");
    else
      inform (loc, "precondition of a call in the predicate cannot be "
	      "proven");
  };
  const obligation &o = obligations[id];
  for (unsigned i = 0; i < o.n_notes; ++i)
    {
      const contract_shadow_note &n = obligation_notes[o.first_note + i];
      note (n.loc, n.always_false);
    }
  for (unsigned d : nested_ids)
    {
      const obligation &n = obligations[d];
      if (n.prove && n.result != CSR_SOUND && n.result != CSR_CONDITIONAL
	  && counts_for_p (d, id))
	note (n.pred_loc != UNKNOWN_LOCATION ? n.pred_loc : n.loc,
	      n.result == CSR_ALWAYS_FALSE);
    }
}

/* Diagnose obligation ID as unproven.  The texts are those of
   c-family/contracts-prove.cc's contract_diagnose_unless_proven.  */

static void
diagnose (unsigned id)
{
  const obligation &o = obligations[id];
  /* E195: a predicate folding found false is always false wherever it is
     evaluated, however the copies' results merged, and also when nothing
     decided the obligation.  */
  bool always_false = o.result == CSR_ALWAYS_FALSE || o.folded_false;
  auto_diagnostic_group d;
  /* DECISIONS.md E186 (a), as in c-family/contracts-prove.cc.  */
  warn_system_headers_sentinel sys (o.pred_loc);
  bool emitted = true;
  if (o.named)
    {
      /* An override pair's (DECISIONS.md E155): at the overriding
	 function's definition, naming the overridden function.  */
      tree base = (*override_bases)[o.named];
      if (o.prove)
	{
	  if (always_false)
	    error_at (o.loc, "contract predicate is always false for a call "
		      "through %qD", base);
	  else
	    error_at (o.loc, "contract assertion cannot be proven for a call "
		      "through %qD", base);
	}
      else if (always_false)
	emitted = warning_at (o.loc, OPT_Wcontract_unproven,
			      "contract predicate is always false for a call "
			      "through %qD", base);
      else
	emitted = warning_at (o.loc, OPT_Wcontract_unproven,
			      "contract assertion cannot be proven for a call "
			      "through %qD", base);
    }
  else if (o.prove)
    {
      if (always_false)
	error_at (o.loc, "contract predicate is always false");
      else
	error_at (o.loc, "contract assertion cannot be proven");
    }
  else if (always_false)
    emitted = warning_at (o.loc, OPT_Wcontract_unproven,
			  "contract predicate is always false");
  else
    emitted = warning_at (o.loc, OPT_Wcontract_unproven,
			  "contract assertion cannot be proven");
  if (emitted && o.pred_loc != UNKNOWN_LOCATION && o.pred_loc != o.loc)
    inform (o.pred_loc, "the predicate of the contract assertion");
  if (emitted)
    inform_nested (id);
}

const pass_data pass_data_contract_shadow_clone =
{
  SIMPLE_IPA_PASS, /* type */
  "contract_shadow_clone", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_CONTRACT_SHADOW, /* tv_id */
  0, /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  0, /* todo_flags_finish */
};

class pass_contract_shadow_clone : public simple_ipa_opt_pass
{
public:
  pass_contract_shadow_clone (gcc::context *ctxt)
    : simple_ipa_opt_pass (pass_data_contract_shadow_clone, ctxt)
  {}

  bool gate (function *) final override { return regions && !in_lto_p; }

  unsigned int execute (function *) final override
  {
    compiled_known = true;
    hash_set<int_hash<int, -1, -2> > compiled;
    auto_vec<cgraph_node *> todo;
    cgraph_node *node;
    FOR_EACH_FUNCTION_WITH_GIMPLE_BODY (node)
      {
	compiled.add (DECL_UID (node->decl));
	compiled.add (DECL_UID (DECL_ORIGIN (node->decl)));
	push_cfun (DECL_STRUCT_FUNCTION (node->decl));
	if (has_regions_p ())
	  todo.safe_push (node);
	note_assumption_fn (node->decl);
	pop_cfun ();
      }
    /* The analysis-only functions' options: the proof level's.  */
    if (contract_shadow_proof_optimization && contract_shadow_level_p ())
      proof_optimization = contract_shadow_proof_optimization ();
    /* The copies first, from the bodies with their regions.  */
    hash_map<cgraph_node *, cgraph_node *> copies, level;
    make_level_copies (todo, level);
    if (premise_regions)
      make_premise_copies (todo, copies, level);
    for (cgraph_node *n : todo)
      process_function (n, copies.is_empty () ? NULL : &copies,
			level.is_empty () ? NULL : &level);
    proof_optimization = NULL_TREE;

    /* An obligation of a compiled function whose marker was not found was
       removed as unreachable: it is never evaluated.  */
    for (unsigned id = 0; id < obligations.length (); ++id)
      {
	obligation &o = obligations[id];
	if (!o.seen && o.result == CSR_PENDING && compiled.contains (o.fn_uid))
	  {
	    o.result = CSR_SOUND;
	    o.unreached = true;
	    if (dump_file)
	      fprintf (dump_file, "contract obligation %u: never evaluated\n",
		       id);
	  }
	/* An override pair's evaluation decided at once is diagnosed only
	   if its function is compiled.  */
	else if (!o.seen && o.named && o.result != CSR_PENDING
		 && compiled.contains (o.fn_uid))
	  o.seen = true;
      }
    /* E169: an obligation whose own marker went (its predicate folded) is
       still reached if the marker of one nested in it was found.  */
    for (unsigned d : nested_ids)
      if (obligations[d].seen && obligations[d].fn_uid != -1)
	for (unsigned p = obligations[d].parent; p;
	     p = obligations[p - 1].parent)
	  obligations[p - 1].unreached = false;
    regions = 0;
    regions_removed = !todo.is_empty ();
    premise_regions = 0;
    return 0;
  }
};

const pass_data pass_data_contract_shadow =
{
  GIMPLE_PASS, /* type */
  "contract_shadow", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_CONTRACT_SHADOW, /* tv_id */
  PROP_cfg, /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  0, /* todo_flags_finish */
};

class pass_contract_shadow : public gimple_opt_pass
{
public:
  pass_contract_shadow (gcc::context *ctxt)
    : gimple_opt_pass (pass_data_contract_shadow, ctxt)
  {}

  bool gate (function *fun) final override
  {
    return ((live_shadows && contract_shadow_function_p (fun->decl))
	    || premise_copy_p (fun->decl) || level_copy_p (fun->decl));
  }

  unsigned int execute (function *fun) final override
  {
    /* A copy is gone with the shadows calling it before it could be
       expanded; should one be, it is discarded too.  */
    if (!premise_copy_p (fun->decl) && !level_copy_p (fun->decl))
      decide_shadow ();
    /* Discarded, never expanded; as for a discarded OpenACC routine, the
       expansion of the function then treats it as written.  */
    TREE_ASM_WRITTEN (fun->decl) = 1;
    return TODO_discard_function;
  }
};

const pass_data pass_data_contract_shadow_early =
{
  SIMPLE_IPA_PASS, /* type */
  "contract_shadow_early", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_CONTRACT_SHADOW, /* tv_id */
  0, /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  0, /* todo_flags_finish */
};

class pass_contract_shadow_early : public simple_ipa_opt_pass
{
public:
  pass_contract_shadow_early (gcc::context *ctxt)
    : simple_ipa_opt_pass (pass_data_contract_shadow_early, ctxt)
  {}

  bool gate (function *) final override
  {
    return ((live_shadows || proven_guards)
	    && (flag_generate_lto || flag_generate_offload));
  }

  unsigned int execute (function *) final override
  {
    auto_vec<cgraph_node *> todo;
    cgraph_node *node;
    delete premise_memo;
    premise_memo = NULL;
    FOR_EACH_DEFINED_FUNCTION (node)
      if (contract_shadow_function_p (node->decl))
	todo.safe_push (node);
    order_shadows (todo);
    for (cgraph_node *n : todo)
      {
	shadow_info *info = shadow_info_of (n->decl);
	if (info->premise_of && !retry_needed_p (info))
	  {
	    skip_retry (n);
	    continue;
	  }
	push_cfun (DECL_STRUCT_FUNCTION (n->decl));
	free_dominance_info (CDI_DOMINATORS);
	free_dominance_info (CDI_POST_DOMINATORS);
	decide_shadow ();
	pop_cfun ();
	n->remove ();
      }
    /* The copies go with the shadows.  */
    if (premise_copies || level_copies)
      {
	auto_vec<cgraph_node *> copies;
	FOR_EACH_FUNCTION (node)
	  if (premise_copy_p (node->decl) || level_copy_p (node->decl))
	    copies.safe_push (node);
	for (cgraph_node *n : copies)
	  n->remove ();
      }

    /* Every obligation is decided: fold the guards of the assumptions made
       of them, in the assumption functions, so that none is streamed.  */
    if (proven_guards)
      {
	FOR_EACH_FUNCTION_WITH_GIMPLE_BODY (node)
	  {
	    push_cfun (DECL_STRUCT_FUNCTION (node->decl));
	    /* Only early inlining is done: a copy that relies on the IPA
	       inliner for what its proof inlined is not assumed.  */
	    if (check_assumption_copies (node))
	      execute_update_addresses_taken ();
	    if (fold_proven_guards ())
	      {
		free_dominance_info (CDI_DOMINATORS);
		free_dominance_info (CDI_POST_DOMINATORS);
		if (current_loops)
		  loops_state_set (LOOPS_NEED_FIXUP);
		cleanup_tree_cfg ();
		cgraph_edge::rebuild_edges ();
	      }
	    pop_cfun ();
	  }
	proven_guards = 0;
      }
    return 0;
  }
};

const pass_data pass_data_contract_proven =
{
  GIMPLE_PASS, /* type */
  "contract_proven", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_CONTRACT_SHADOW, /* tv_id */
  PROP_cfg, /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  0, /* todo_flags_finish */
};

/* Fold the .CONTRACT_PROVEN guards of a function -- an assumption function
   -- about to be optimized after IPA, every shadow having been decided
   before (contract_shadow_expand_first).  */

class pass_contract_proven : public gimple_opt_pass
{
public:
  pass_contract_proven (gcc::context *ctxt)
    : gimple_opt_pass (pass_data_contract_proven, ctxt)
  {}

  bool gate (function *) final override
  {
    return (proven_guards || assumption_fns
	    || ((premise_fns || level_copies) && live_shadows));
  }

  unsigned int execute (function *fun) final override
  {
    if (live_shadows && contract_shadow_function_p (fun->decl))
      {
	note_undecided_premise (fun->decl);
	return 0;
      }
    unsigned todo = 0;
    if (check_assumption_copies (cgraph_node::get (current_function_decl)))
      todo |= TODO_update_address_taken;
    if (!fold_proven_guards ())
      return todo;
    if (current_loops)
      loops_state_set (LOOPS_NEED_FIXUP);
    return todo | TODO_cleanup_cfg;
  }
};

} // anon namespace

int
contract_shadow_expand_first ()
{
  if (!live_shadows)
    return 0;
  /* The target's RTL machinery is set up when the first function is
     expanded, with that function's options: whether the hard frame pointer
     may be allocated follows its -fomit-frame-pointer, for one.  That must
     not be a shadow, whose options are the proof level's (at proof level 0
     in an optimized compilation, -O0's, which keep the frame pointer); set
     it up now, with the command line's, as the first real function would
     without the shadows.  */
  initialize_rtl ();
  delete premise_memo;
  premise_memo = NULL;
  auto_vec<cgraph_node *> todo;
  cgraph_node *node;
  FOR_EACH_DEFINED_FUNCTION (node)
    if (node->process && contract_shadow_function_p (node->decl))
      todo.safe_push (node);
  order_shadows (todo);
  for (cgraph_node *n : todo)
    {
      /* A premise shadow is optimized further only if the ordinary shadow
	 it retries, decided by now, left something unproven.  */
      shadow_info *info = shadow_info_of (n->decl);
      if (info->premise_of && !retry_needed_p (info))
	{
	  skip_retry (n);
	  continue;
	}
      n->process = 0;
      n->expand ();
    }
  /* Then the assumption functions of the obligations, whose guards are now
     decided: the ranger uses an assumption only once its function has been
     optimized (pass_assumptions), so the real functions must come after
     them, which their order otherwise need not ensure.  */
  if (assumption_fns)
    {
      auto_vec<cgraph_node *> assumptions;
      FOR_EACH_DEFINED_FUNCTION (node)
	if (node->process
	    && assumption_fn_id (node->decl))
	  assumptions.safe_push (node);
      for (cgraph_node *n : assumptions)
	{
	  n->process = 0;
	  n->expand ();
	}
    }
  return todo.length ();
}

void (*contract_shadow_diagnostic_context) (unsigned, bool);

/* True if no obligation obligation ID is nested in is an override pair's
   that is not diagnosed because its function is not compiled here
   (DECISIONS.md E150).  */

static bool
compiled_parent_p (unsigned id)
{
  for (unsigned p = obligations[id].parent; p; p = obligations[p - 1].parent)
    if (obligations[p - 1].named && !obligations[p - 1].seen)
      return false;
  return true;
}

/* DECISIONS.md E169 (9): obligation ID, with the result R, is about to be
   diagnosed.  A nested evaluation with diagnose made for the analysis
   evaluation of a contract assertion and an evaluation made for the
   runtime check of the same assertion that follows it, at the same call
   in its predicate, with the same diagnostic, are diagnosed once: return
   true if ID is the second.  REPORTED holds the ids of the first kind
   diagnosed so far and not yet matched, in order.  The obligations are
   diagnosed in the order they were registered, so the analysis
   evaluation's (registered first) always come first.  */

static bool
reported_once_p (unsigned id, contract_shadow_result r,
		 vec<unsigned> &reported)
{
  const obligation &o = obligations[id];
  if (!o.once_group || o.prove)
    return false;
  bool always_false = r == CSR_ALWAYS_FALSE || o.folded_false;
  if (!o.once_check)
    {
      if (o.parent)
	reported.safe_push (id);
      return false;
    }
  for (unsigned i = 0; i < reported.length (); ++i)
    {
      const obligation &a = obligations[reported[i]];
      if (a.once_group == o.once_group
	  && a.loc == o.loc
	  && a.pred_loc == o.pred_loc
	  && a.named == o.named
	  && (effective_result (reported[i]) == CSR_ALWAYS_FALSE
	      || a.folded_false) == always_false)
	{
	  reported.ordered_remove (i);
	  return true;
	}
    }
  return false;
}

void
contract_shadow_finish ()
{
  /* The context of each diagnostic is the function holding the evaluation,
     as if it were being compiled ("In function ..."), and whatever else the
     front end kept of the context it was described in.  */
  tree saved = current_function_decl;
  /* After an error the middle end does not run (symbol_table::compile), so
     contract_shadow_clone never found out which functions are compiled.
     The symbol table still holds the definitions this translation unit
     would emit: an override pair's evaluations in those are diagnosed as
     without the error -- one decided at once (folding, which is all there
     is without override shadows) as decided, one left pending as
     unproven, as every other pending obligation is.  */
  if (!compiled_known)
    {
      hash_set<int_hash<int, -1, -2> > compiled;
      cgraph_node *node;
      FOR_EACH_DEFINED_FUNCTION (node)
	{
	  compiled.add (DECL_UID (node->decl));
	  compiled.add (DECL_UID (DECL_ORIGIN (node->decl)));
	}
      for (obligation &o : obligations)
	if (o.named && !o.seen && compiled.contains (o.fn_uid))
	  o.seen = true;
    }
  auto_vec<unsigned> once_reported;
  for (unsigned id = 0; id < obligations.length (); ++id)
    {
      obligation &o = obligations[id];
      /* An override pair's obligation in a function never compiled here
	 (DECISIONS.md E150): the translation unit that emits the function
	 -- and the vtable through which calls via the overridden function
	 reach it -- analyzes it.  */
      if (o.named && !o.seen)
	continue;
      /* E169: one nested with prove is noted with those it is nested in;
	 nothing nested in one never evaluated is diagnosed, nor in an
	 override pair's obligation that is not (E150).  */
      if (o.parent && (o.prove || !reached_p (id) || !compiled_parent_p (id)))
	continue;
      contract_shadow_result r = effective_result (id);
      if (r != CSR_SOUND && r != CSR_CONDITIONAL
	  && !reported_once_p (id, r, once_reported))
	{
	  current_function_decl = (*obligation_fns)[id];
	  bool context = o.context && contract_shadow_diagnostic_context;
	  if (context)
	    contract_shadow_diagnostic_context (o.context, true);
	  diagnose (id);
	  if (context)
	    contract_shadow_diagnostic_context (o.context, false);
	}
    }
  current_function_decl = saved;
  obligations.release ();
  nested_ids.release ();
  obligation_notes.release ();
  obligation_fns = NULL;
  obligation_callees = NULL;
  assumption_callees.release ();
  for (shadow_info *s : shadows)
    delete s;
  shadows.release ();
  shadow_real_fns = NULL;
  override_bases = NULL;
  live_shadows = 0;
  regions = 0;
  regions_removed = false;
  proven_guards = 0;
  compiled_known = false;
  delete premise_fns;
  premise_fns = NULL;
  obligation_sites.release ();
  delete assumption_fns;
  assumption_fns = NULL;
  delete premise_memo;
  premise_memo = NULL;
  premise_regions = 0;
  premise_copies = 0;
  level_copies = 0;
  shadow_decisions.release ();
}

void
contract_shadow_note_inline (cgraph_node *dst, tree fn, gimple *stmt)
{
  bool premise = premise_fn_p (fn);
  tree decl = dst->decl;
  if (!contract_shadow_function_p (decl))
    {
      /* DST's body now holds FN's code and what follows from it.  */
      if (premise)
	contract_shadow_note_premise (decl);
      return;
    }
  /* A refinable body holds evaluations another definition may make
     differently even where they leave no premise here: one with ignore
     has no effects, another TU's with observe calls a violation handler,
     which may store to memory the shadow then takes as unchanged.  The
     mark follows the code through inlining, as noted premises do.  */
  if ((!premise && !refinable_body_p (fn)) || !replaceable_origin_p (fn))
    return;
  /* The calls from the shadow's body down to this one: each must be
     inlined in the real function for its code to be this one.  */
  shadow_info *info = shadow_info_of (decl);
  for (tree b = gimple_block (stmt); b && TREE_CODE (b) == BLOCK;
       b = BLOCK_SUPERCONTEXT (b))
    {
      int uid = inlined_fn_uid (b);
      if (uid >= 0)
	{
	  inline_site site
	    = { uid, LOCATION_LOCUS (BLOCK_SOURCE_LOCATION (b)), false };
	  info->sites.safe_push (site);
	}
    }
  inline_site site = { DECL_UID (DECL_ORIGIN (fn)), call_locus (stmt, fn),
		       true };
  info->sites.safe_push (site);
}

simple_ipa_opt_pass *
make_pass_contract_shadow_clone (gcc::context *ctxt)
{
  return new pass_contract_shadow_clone (ctxt);
}

gimple_opt_pass *
make_pass_contract_shadow (gcc::context *ctxt)
{
  return new pass_contract_shadow (ctxt);
}

simple_ipa_opt_pass *
make_pass_contract_shadow_early (gcc::context *ctxt)
{
  return new pass_contract_shadow_early (ctxt);
}

gimple_opt_pass *
make_pass_contract_proven (gcc::context *ctxt)
{
  return new pass_contract_proven (ctxt);
}

#include "gt-contracts-shadow.h"
