---
id: 4270-p3400-bypass-rethrowing-local-handler
subject: 'c++: contracts: bypass a local handler that only rethrows'
depends:
  - 4200-p3400-core
  - 4260-p3400-facet-local-violation-handler
regenerates: []
fixes: []
---

## Rationale

The optimization that keeps a label's local violation handler from
destroying the exception it was only ever going to rethrow, and the
`-fcontract-bypass-rethrowing-local-handler` switch that governs it.

The problem is structural.  When a predicate throws under `enforce` or
`observe`, the emitted check has to catch the exception in order to build an
`evaluation_exception` and offer it to the handler; the handler's usual
answer is to rethrow.  Catch-and-rethrow is not free and not transparent:
the original exception has already been caught, so the propagating exception
is a fresh throw from a different frame, and the observable stack, the
`std::uncaught_exceptions()` count and any intervening cleanup all differ
from what would have happened had it simply propagated.  When the handler is
*provably* going to rethrow, the whole catch can be elided and the
predicate's exception left to propagate on its own.

Proving it is the bulk of this commit: an abstract interpreter over the
resolved handler's body.

* `aval` / `aval_kind` is a three-point abstract domain -- unknown, a known
  integer/enum/bool constant, and "the result of `std::current_exception()`"
  -- which is exactly enough to follow the two shapes that matter, a handler
  that tests the violation's detection mode and one that stashes and
  rethrows the current exception.

* `rethrow_outcome` is the per-statement answer: fell through, rethrew,
  returned without doing anything observable, or could not be analysed.
  `RO_FAIL` is the safe answer and every unhandled construct produces it.

* `rethrow_analysis` walks the body, following calls up to
  `RETHROW_MAX_DEPTH` (which also terminates mutual recursion), with
  `accessor_value` folding member reads of the violation parameter so that
  `if (v.detection_mode() == ...)` can be decided statically, and
  `calls_std_fn_p` / `strip_to_decl` recognising the library calls the domain
  knows about.

* `contract_local_handler_always_rethrows_p` is the entry point.  It mirrors
  the exact conditions under which `build_contract_data_block_ctor` records
  a local handler -- if there is no handler there is nothing to reason about
  -- and requires the *resolved* callee that
  `build_local_violation_trampoline` recorded, so a virtual handler, whose
  callee is not known, is declined rather than guessed at.

Everything but the entry point is in an anonymous namespace: none of it is
usable, or meaningful, outside this one question.

The switch defaults on.  It exists because the transformation is observable
in exactly the way described above, so a program that depends on the
catch-and-rethrow shape needs a way to say so.

## Compile gap

`contract_local_handler_always_rethrows_p` reads
`local_violation_trampoline_map` and `local_violation_handler_fn_map`, both
populated by `4260-p3400-facet-local-violation-handler`, and needs the
resolved-callee slot that commit fills.  Its call site is this commit's
own: the `gcc/cp/contracts.cc#200` cut declared in
`branch-history/splits.md` gives it the seven lines of
`emit_check_for_semantic` that clear `check_might_throw` when the analysis
succeeds, so the optimization is wired up here rather than sitting compiled
and unreferenced until some later emitter rewrite.

Two traps.

The first is that this is a pure optimization with no diagnostic surface,
so an unwired analysis is invisible in every functional test: the check
still catches, the handler still runs, the handler still rethrows, and the
program still behaves correctly.  Only the *shape* differs.  That is why the
tests in this commit are a graded series over handler bodies rather than
one behavioural test, and why several of them scan the generated code
instead of observing an effect.

The second is the direction of the failure.  `RO_FAIL` is the conservative
answer -- keep the catch -- so an analysis that is merely incomplete costs
nothing.  An analysis that is *wrong* in the other direction elides a catch
around a handler that does not rethrow, and the violation is then never
reported at all.  Every construct the walker does not understand must
therefore reach `RO_FAIL`, which is the invariant
`p3400-bypass-rethrowing-local-handler-4.C` through `-6.C` exist to hold.

## Contents

- gcc/c-family/c.opt : /^fcontract-bypass-rethrowing-local-handler$/
- gcc/cp/contracts.cc : @RETHROW_MAX_DEPTH, @accessor_value, @aval, @aval_kind, @calls_std_fn_p, @contract_local_handler_always_rethrows_p:0, #202:1, @namespace, @private, @public, @rethrow_analysis, @rethrow_outcome, @strip_to_decl
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-1.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-10.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-2.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-3.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-4.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-5.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-6.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-7.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-8.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-9.C : *
