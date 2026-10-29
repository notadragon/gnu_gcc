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
   whose predicate has no side effects, get a region too, ending in
   .CONTRACT_ASSUMPTION (guaranteed) -- guaranteed for assume.  One with
   enforce, noexcept_enforce or quick_enforce needs none: its check calls a
   noreturn entry point or traps, which ensures the predicate on the
   continuing path.

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

   2. The shadow is optimized by whatever pipeline the function's own -O
      level runs, inlining included.  Whatever the optimizers can show
      unreachable is proven -- undefined behavior included, which is
      treated as unreachable exactly like an assumed predicate.  Warnings
      are inhibited while a shadow is the current function.

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
   have side effects (an obligation's: the shadow's later state may then
   differ from the program's), or when any of its obligations is unproven
   (each marker is noreturn, so what follows it assumes its predicate).
   Undefined behavior is a guaranteed premise: it does not make a proof
   conditional.  Both suppress the diagnostic; the distinction is recorded
   per obligation (contract_shadow_obligation_result).  A predicate's side
   effects are judged when the shadow is decided: one that calls functions
   directly has none of its own if, by then, each of them is known to be
   const or pure, terminating and not throwing -- which the IPA analyses
   often find of an inline function not declared so (std::vector::size).
   Anything else with side effects (a store, a volatile access, an indirect
   or virtual call, a temporary) counts.

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

   A marker duplicated by unrolling, threading or the C++ constructor and
   destructor clones is one obligation: an obligation is unproven if any
   reachable copy is.  "Always false" needs the marker reached from the
   entry without a condition and outside any loop when the shadow was made
   (a copy of another iteration may have been proven and removed).

   Shadows keep the functions their bodies reference alive until inlining
   is done, then only as declarations (contract_shadow_boundary_p), so
   nothing is emitted for them.

   The diagnostics are issued in registration order at the end of the
   compilation (contract_shadow_finish), so that an error found in one
   shadow cannot stop the passes that optimize another; so are those of
   the evaluations a front-end strategy decided while shadows may be
   active (contract_shadow_defer_diagnostic), since the middle end does not
   run at all after an error.  Each is issued in the context its evaluation
   was described in: in the function holding it ("In function ..."), and
   in whatever context the front end kept for it
   (contract_shadow_diagnostic_context: C++'s instantiation backtrace).

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
};

/* Every obligation, in registration (source) order.  */
static vec<obligation> obligations;
/* Every shadow, indexed by the value of its "contract shadow" attribute.  */
static vec<shadow_info *> shadows;
/* Shadows not yet diagnosed.  */
static unsigned live_shadows;
/* Regions placed by the front ends.  */
static unsigned regions;
/* .CONTRACT_PROVEN guards placed by the front ends and not yet folded
   everywhere.  */
static unsigned proven_guards;
/* contract_shadow_clone ran, and so found out which functions are
   compiled.  */
static bool compiled_known;

/* The internal attribute marking a shadow.  The space keeps it out of
   reach of source code.  */
static const char shadow_attr[] = "contract shadow";

} // anon namespace

/* Per obligation, the function whose body holds it, for the context of its
   diagnostic.  */
static GTY(()) vec<tree, va_gc> *obligation_fns;
/* The functions the obligations' predicates call, kept until the shadows
   holding them are decided.  */
static GTY(()) vec<tree, va_gc> *obligation_callees;
/* Per shadow, as for shadows, the function it is a copy of.  */
static GTY(()) vec<tree, va_gc> *shadow_real_fns;
/* Per override pair, the overridden function; element 0 is unused.  */
static GTY(()) vec<tree, va_gc> *override_bases;

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
				     unsigned pair)
{
  obligation o = {};
  o.context = context;
  o.pair = pair;
  o.named = named;
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

void
contract_shadow_note_proven_guard ()
{
  ++proven_guards;
}

void
contract_shadow_defer_diagnostic (tree fndecl, location_t loc,
				  location_t pred_loc, bool prove,
				  bool always_false, unsigned context,
				  unsigned pair)
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

bool
contract_shadow_pending_p ()
{
  for (const obligation &o : obligations)
    if (o.result == CSR_PENDING)
      return true;
  return false;
}

contract_shadow_result
contract_shadow_obligation_result (unsigned id)
{
  gcc_checking_assert (id < obligations.length ());
  return obligations[id].result;
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
  if (!live_shadows || symtab->state < IPA_SSA_AFTER_INLINING)
    return false;
  if (node->inlined_to)
    node = node->inlined_to;
  return contract_shadow_function_p (node->decl);
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

/* True if evaluating the predicate of obligation O may have side effects,
   as far as is known now: it has some of its own, or it calls a function
   not known to be const or pure, terminating and not throwing.  */

static bool
side_effects_p (const obligation &o)
{
  if (o.side_effects)
    return true;
  for (unsigned i = 0; i < o.n_callees; ++i)
    {
      tree callee = (*obligation_callees)[o.first_callee + i];
      int flags = flags_from_decl_or_type (callee);
      if (!(flags & (ECF_CONST | ECF_PURE))
	  || (flags & ECF_LOOPING_CONST_OR_PURE)
	  || (!(flags & ECF_NOTHROW) && flag_exceptions))
	return true;
    }
  return false;
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
   result to its value, true or false accordingly.  */

static void
fold_guard_conditions (hash_map<tree, bool> &guards)
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

/* True if a region of kind KIND is executed in the shadow INFO: an ordinary
   region in every shadow, an override region only in its pair's.  */

static bool
region_active_p (unsigned kind, const shadow_info *info)
{
  return kind == 0 || kind == info->pair;
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
   kinds that got a shadow, holds theirs.  */

static void
fold_regions (shadow_info *info, const vec<unsigned> &shadowed)
{
  hash_map<tree, bool> guards;
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
	    replace_guard (&gsi,
			   info && region_active_p (region_kind (stmt, 0), info),
			   guards);
	    break;
	  case IFN_CONTRACT_UNPROVEN:
	    {
	      unsigned id = tree_to_uhwi (gimple_call_arg (stmt, 0));
	      gcc_checking_assert (id < obligations.length ());
	      obligation &o = obligations[id];
	      if (!info)
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
		     its predicate, as after any marker.  */
		  if (!relied.add (id))
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
	    if (info && region_active_p (region_kind (stmt, 1), info))
	      {
		if (integer_zerop (gimple_call_arg (stmt, 0)))
		  info->conditional = true;
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
    fold_guard_conditions (guards);

  free_dominance_info (CDI_DOMINATORS);
  free_dominance_info (CDI_POST_DOMINATORS);
  if (current_loops)
    loops_state_set (LOOPS_NEED_FIXUP);
  cleanup_tree_cfg ();
  cgraph_edge::rebuild_edges ();
}

/* True if the current function's body can be copied into a shadow and the
   shadow optimized; NODE is its cgraph node.  */

static bool
shadowable_p (cgraph_node *node)
{
  tree decl = node->decl;
  return (opt_for_fn (decl, optimize) > 0
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
   INDEX and what keeps the shadow to itself.  */

static tree
shadow_attributes (tree attrs, unsigned index)
{
  static const char *const dropped[] = {
    "always_inline", "gnu_inline", "noinline", "noclone", "no_icf",
    "target_clones", "target_version", "alias", "ifunc", "weak", "weakref",
    "used", "retain", "externally_visible", "symver", "constructor",
    "destructor", "omp declare simd", "omp declare target",
    "omp declare target block", "omp declare variant base"
  };
  tree result = NULL_TREE;
  for (tree a = attrs; a; a = TREE_CHAIN (a))
    {
      bool drop = false;
      for (const char *name : dropped)
	if (is_attribute_p (name, get_attribute_name (a)))
	  drop = true;
      if (!drop)
	result = tree_cons (TREE_PURPOSE (a), TREE_VALUE (a), result);
    }
  result = nreverse (result);
  result = tree_cons (get_identifier ("noinline"), NULL_TREE, result);
  result = tree_cons (get_identifier ("noclone"), NULL_TREE, result);
  result = tree_cons (get_identifier ("no_icf"), NULL_TREE, result);
  result = tree_cons (get_identifier (shadow_attr),
		      build_tree_list (NULL_TREE,
				       build_int_cst (unsigned_type_node,
						      index)),
		      result);
  return result;
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
   holds no obligation.  */

static void
process_function (cgraph_node *node)
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
	cgraph_node *shadow
	  = node->create_version_clone_with_body (vNULL, NULL, NULL, NULL, NULL,
						  kind ? "contract_override"
						  : "contract_shadow");
	if (!shadow)
	  continue;
	tree sdecl = shadow->decl;
	DECL_ATTRIBUTES (sdecl)
	  = shadow_attributes (DECL_ATTRIBUTES (decl), index);
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

  if (dump_file)
    {
      for (unsigned i = 0; i < made.length (); ++i)
	if (shadowed[i])
	  fprintf (dump_file, "Override shadow %s of %s for pair %u\n",
		   made[i]->dump_name (), node->dump_name (), shadowed[i]);
	else
	  fprintf (dump_file, "Shadow %s of %s\n", made[i]->dump_name (),
		   node->dump_name ());
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
  if (!premise_fns)
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
   the override shadows, which may take the ordinary obligations as
   premises (decide_shadow), keeping the order otherwise.  */

static void
order_shadows (vec<cgraph_node *> &todo)
{
  auto_vec<cgraph_node *> overrides;
  unsigned n = 0;
  for (cgraph_node *node : todo)
    if (shadow_info_of (node->decl)->pair)
      overrides.safe_push (node);
    else
      todo[n++] = node;
  todo.truncate (n);
  for (cgraph_node *node : overrides)
    todo.quick_push (node);
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
     guarded it, so the predicate is false whenever it is evaluated.  */
  auto unconditional_p = [] (basic_block target) {
    basic_block b = ENTRY_BLOCK_PTR_FOR_FN (cfun);
    for (int n = n_basic_blocks_for_fn (cfun); n > 0; --n)
      {
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

  /* What the shadow relies on that may not hold in the program.  */
  bool premises = info->conditional;
  for (unsigned id : info->ids)
    if (side_effects_p (obligations[id]))
      premises = true;
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
    if (obligations[id].result != CSR_SOUND)
      {
	premises = true;
	if (dump_file)
	  fprintf (dump_file, "Relies on contract obligation %u, which is "
		   "%s\n", id, result_name (obligations[id].result));
	break;
      }
  bool conditional = premises || !unproven.is_empty ();
  for (unsigned id : info->ids)
    {
      contract_shadow_result r;
      if (unproven.contains (id))
	r = (always_false.contains (id) && !premises
	     && !info->in_loop.contains (id)
	     ? CSR_ALWAYS_FALSE : CSR_UNPROVEN);
      else
	r = conditional ? CSR_CONDITIONAL : CSR_SOUND;
      merge_result (obligations[id], r);
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
			&& obligations[id].result == CSR_SOUND);
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
  unsigned *id = assumption_fns->get (DECL_UID (TREE_OPERAND (a, 0)));
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
	    || obligations[id].result != CSR_SOUND)
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

/* Diagnose obligation O as unproven.  The texts are those of
   c-family/contracts-prove.cc's contract_diagnose_unless_proven.  */

static void
diagnose (const obligation &o)
{
  bool always_false = o.result == CSR_ALWAYS_FALSE;
  auto_diagnostic_group d;
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
    for (cgraph_node *n : todo)
      process_function (n);

    /* An obligation of a compiled function whose marker was not found was
       removed as unreachable: it is never evaluated.  */
    for (unsigned id = 0; id < obligations.length (); ++id)
      {
	obligation &o = obligations[id];
	if (!o.seen && o.result == CSR_PENDING && compiled.contains (o.fn_uid))
	  {
	    o.result = CSR_SOUND;
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
    regions = 0;
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
    return live_shadows && contract_shadow_function_p (fun->decl);
  }

  unsigned int execute (function *fun) final override
  {
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
	push_cfun (DECL_STRUCT_FUNCTION (n->decl));
	free_dominance_info (CDI_DOMINATORS);
	free_dominance_info (CDI_POST_DOMINATORS);
	decide_shadow ();
	pop_cfun ();
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
    return proven_guards || assumption_fns || (premise_fns && live_shadows);
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
	    && assumption_fns->get (DECL_UID (node->decl)))
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
  for (unsigned id = 0; id < obligations.length (); ++id)
    {
      obligation &o = obligations[id];
      /* An override pair's obligation in a function never compiled here
	 (DECISIONS.md E150): the translation unit that emits the function
	 -- and the vtable through which calls via the overridden function
	 reach it -- analyzes it.  */
      if (o.named && !o.seen)
	continue;
      if (o.result != CSR_SOUND && o.result != CSR_CONDITIONAL)
	{
	  current_function_decl = (*obligation_fns)[id];
	  bool context = o.context && contract_shadow_diagnostic_context;
	  if (context)
	    contract_shadow_diagnostic_context (o.context, true);
	  diagnose (o);
	  if (context)
	    contract_shadow_diagnostic_context (o.context, false);
	}
    }
  current_function_decl = saved;
  obligations.release ();
  obligation_fns = NULL;
  obligation_callees = NULL;
  for (shadow_info *s : shadows)
    delete s;
  shadows.release ();
  shadow_real_fns = NULL;
  override_bases = NULL;
  live_shadows = 0;
  regions = 0;
  proven_guards = 0;
  compiled_known = false;
  delete premise_fns;
  premise_fns = NULL;
  obligation_sites.release ();
  delete assumption_fns;
  assumption_fns = NULL;
  delete premise_memo;
  premise_memo = NULL;
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
