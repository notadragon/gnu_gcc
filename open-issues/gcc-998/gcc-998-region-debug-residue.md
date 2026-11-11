# GCC-998: with -g, the regions leave traces in the real code's debug information

**Kind:** deferred (a documented limitation) -- deferred by the user
2026-11-08 (DECISIONS.md E209 (c))
**Status:** Open, this branch only -- recorded 2026-11-08 (X14-GCC, the
GCC-997 fix); documented in `gcc/doc/invoke.texi` under
`-fcontracts-prove-shadow`
**Affects:** the P4405 regions the front ends place for the analysis
evaluations (`c-family/contracts-prove-shadow.cc`, `cp/contracts.cc`), and
what the middle end describes of the real function before
`contract_shadow_clone` removes them: the early debug information
(`dwarf2out_early_global_decl`, from `finalize_compilation_unit`), the CFG
built and cleaned up with the regions (`build_gimple_cfg`).  The code is
not affected.
**Clang:** CLANG-916 is the counterpart of the first shape (types and
functions only a predicate uses described), there from the assumption
regions under `-fcontracts-prove-assume` only -- open, DECISIONS.md E211;
CLANG-915, what Clang's shadow phase itself left (fixed), is the part GCC
had no counterpart of after GCC-997.  The lost statement markers and the
override snapshot's scope do not reproduce in Clang (measured 2026-11-09,
PO7-Clang)

## Symptom

With `-g -gno-record-gcc-switches -fno-contracts-prove-assume`,
`diagnose`, the debug information of the real functions differs from
proof level 0 (`-fcontracts-prove-optimize=0`, no region placed) and from
`-fno-contracts-prove-shadow`, though the code does not:

* [`open-bug-p4405-shadow-debug-info-residue.C`](../../gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-shadow-debug-info-residue.C)
  (-O0 and -O1):
  * a type or function only a predicate uses is described: the class of a
    temporary and its destructor's concrete instance (`f10`,
    `T (n).ok ()`), a lambda's closure type (`f11`), the declaration of a
    function called only in a predicate (`f13`), the constructor and `ok`
    of an explicit conversion's type (`f15`'s `U (n)`, whose constructor
    is only declared; `f16`'s `W<int> (n)`; added 2026-11-09, X16-GCC,
    from Clang's CLANG-915 shapes) -- at every level;
  * from -Og up, the debug markers (`DEBUG BEGIN_STMT`, `.loc ... view`)
    of the statements in an arm that holds a region are lost where they
    are kept without the regions: both arms of an `if` with an `else`
    (`f5`), a `switch` case (`f8`), a loop body (`f9`, at -Og and -O1).
* [`open-bug-p4405-shadow-debug-info-scope.C`](../../gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-shadow-debug-info-scope.C)
  (-O0 and -O2): an override shadow that needs a snapshot of a parameter
  the body modifies (`D::get` assigns `x`, which `B::get`'s postcondition
  reads) describes `D::get`'s local `r` in a `DW_TAG_lexical_block`,
  where without the override regions it is in the function's outermost
  scope.

What GCC-997 covered -- the analysis-only functions themselves (abstract
instances, `DW_AT_abstract_origin`), the discriminators the regions took,
and every other shape of `p4405-prove-optimize-same-code-O0.C` -- is
identical (`p4405-shadow-debug-info*.C`).  So is
an external vtable a predicate loads (`use`'s qualified call
`v->V::ok (n)` in `p4405-shadow-debug-info.C`), which Clang's shadows
described before CLANG-915's fix.

## Causes

All three are the same thing seen from three places: the real function is
described, or first lowered, while it still holds its regions, which only
`contract_shadow_clone` (the first IPA pass) removes.

1. Early debug.  `finalize_compilation_unit` emits the early DWARF of every
   function before `compile ()`: the function's BLOCK tree with the
   regions' locals and types, and every function the cgraph knows of
   (the predicate's callees, a temporary's destructor).  Nothing removes
   those DIEs when the regions go.
2. CFG build.  `build_gimple_cfg` cleans up the CFG with the regions in
   it.  An arm holding only the statement's debug marker and a region
   shares its join with the region's exits, so the forwarder holding the
   marker is removed while its predecessor still has two successors and
   the marker is dropped (`move_debug_stmts_from_forwarder`); without the
   regions the condition collapses first and the marker moves to the
   predecessor.  `c_contract_hoist_region_guards` does not help: giving
   the real function `if (c) <markers>` in the guard's else arm has the
   same shared join at CFG-build time (tried in X14-GCC).
3. Scope.  `place_override_shadows_alone` declares the snapshots in a
   compound statement of its own around the body, so the body's outermost
   scope becomes a nested BLOCK (a function with its own postconditions
   gets the block `maybe_apply_function_contracts` builds; not measured).

## Possible fixes

* (a) Make the shadows, and remove the regions from the real function,
  before early debug and before the CFG is built: version the functions
  holding regions at the end of `analyze_functions` (GIMPLE without a
  CFG), give the real function the region-free body, and build both CFGs
  after.  Fixes 1 and 2 at the root; a larger change to
  `contract_shadow_clone`'s placement (it relies on cgraph edges and the
  IPA pass manager).
* (b) Per shape: drop the early DIEs of entities only the regions
  reference (needs knowing, at early-debug time, which references are in
  regions -- a walk of each body); in `assign_discriminators`-style, teach
  the build-time CFG cleanup to treat region blocks as absent; flatten the
  snapshots' scope in the real function after the regions are removed
  (`fold_regions`: merge the body's BLOCK into the outermost one when the
  latter holds only the snapshots).  Three separate patches, each
  reaching into an upstream file.
* (c) Document it as a limitation (as GCC-995): with `-g` the debug
  information of a function holding analysis evaluations can describe
  entities only its predicates use and can lose statement markers; the
  code is unaffected.

Recommendation: (c) for now, (a) if debug-info identity is wanted later --
(b)'s per-shape patches would each need its own maintenance against
upstream.

## Decision

(c), DECISIONS.md E209, 2026-11-08: documented and left, as GCC-995 was
(E204 (a)).  `invoke.texi` says, under `-fcontracts-prove-shadow`, next
to GCC-995's paragraph, that with `-g` the debug information can still
differ while the code does not, with the three shapes.  The watch tests
stay, xfailed, as the record of the current behaviour (they XPASS if the
regions stop leaving traces).  Revisit with (a) if identical debug
information turns out to matter.
