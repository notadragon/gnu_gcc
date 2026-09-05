# GCC-5: Contract on a function returning a class with a non-trivial destructor double-destroys the return value

**Status:** Fixed here (commit `6e5a47c2a39`)
**Component:** c++ / contracts
**Upstream Link:** None found. Searched 2026-09-05, including resolved
bugs, on comment text `contract return value destroyed twice`
**Affects:** stock g++ 16.2.0, trunk; also reproduces on this branch's
pre-fix baseline, whose relevant functions are byte-identical to trunk; not
independently confirmed on public Compiler Explorer

## Bug Report

A contract on a function returning a class with a non-trivial destructor
causes the return-value cleanup to be emitted twice, because the contract
wrapper re-triggers `maybe_splice_retval_cleanup`'s `sk_function_parms` test.
Depending on the shape this ICEs, or silently double-destroys the returned
object at runtime.

## Reproducer

See [`gcc-05-calendar-gimplify-ice.cpp`](gcc-05-calendar-gimplify-ice.cpp) and
[`gcc-05b-contract-retval-double-destroy.cpp`](gcc-05b-contract-retval-double-destroy.cpp)
in this directory.

## Our Fix

`gcc/cp/contracts.cc`: hide `current_retval_sentinel` across the artificial
contract-check block (via `make_temp_override`) so the splice logic takes
its already-spliced early exit instead of re-emitting the cleanup.

## Notes

Not yet filed upstream; no known duplicate bug number.
