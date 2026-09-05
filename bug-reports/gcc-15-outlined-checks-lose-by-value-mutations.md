# GCC-15: Outlined contract checks lose by-value mutations under `-fcontract-checks-outlined`

**Status:** Fixed here (commit `0377d4408ab`)
**Component:** c++ / contracts
**Upstream Link:** None found. Searched 2026-09-05, including resolved
bugs, on comment text `contract outlined check parameter mutation`
**Affects:** stock g++ 16.2.0, trunk

## Bug Report

Under `-fcontract-checks-outlined`, a predicate mutating a by-value
parameter or postcondition result (via `const_cast`) writes to a copy passed
into the outlined `__pre_fn`/`__post_fn`, so the mutation is silently lost,
while the same predicate inlined observes it. This makes a conforming
program's behavior depend on a codegen flag. This fix retires the upstream
`expr.prim.id.unqual.p7-3.C` xfail.

## Reproducer

See [`gcc-15-outlined-checks-lose-by-value-mutations.cpp`](gcc-15-outlined-checks-lose-by-value-mutations.cpp)
in this directory.

## Our Fix

`gcc/cp/contracts.cc` (`build_contract_condition_function`): pass by-value
parameters by reference into the outlined check functions.

## Notes

This fix retires the upstream `expr.prim.id.unqual.p7-3.C` xfail, which is
worth noting explicitly when this is filed or referenced upstream.
