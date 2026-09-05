# GCC-31: A lambda in a contract predicate cannot capture the enclosing function's parameters

**Status:** Fixed here (commit `e3c949476e4`)
**Component:** c++ / contracts, lambdas
**Upstream Link:** UNKNOWN -- not yet filed
**Affects:** measured 2026-09-05 -- reproduces on stock g++ trunk with plain
`-fcontracts`

## Bug Report

```cpp
void f (int x) pre ([x] { return x > 0; } ()) { }   // rejected
```

```
error: use of parameter outside function body before '>' token
```

All three capture forms are rejected -- `[x]`, `[=]` and `[&]` -- in a
precondition, in a postcondition, and in an assertion-statement alike.

[expr.prim.lambda.capture]/3.3 allows a capture-default or simple-capture in
a lambda-introducer when the lambda appears within a contract assertion and
its innermost enclosing scope is the corresponding contract-assertion scope.
These programs are well-formed.

The diagnostic is the tell: "use of parameter outside function body" is the
message for naming a parameter in a context that is not the function's body
at all, so the contract-assertion scope is not being treated as one from
which the enclosing parameters are reachable.

## Reproducer

See [`gcc-31-lambda-capture-in-contract-predicate.cpp`](gcc-31-lambda-capture-in-contract-predicate.cpp)
in this directory. Two controls bound it:

* a lambda in a predicate that captures **nothing** was always accepted, so
  the defect is in the capture rather than in allowing a lambda in a
  predicate; and
* the **same capturing lambda in the function body** is accepted, so it is
  the contract-assertion scope that is mishandled, not the lambda.

The postcondition rows deliberately take a `const` parameter: a postcondition
that odr-uses a by-value parameter needs it `const` under
[dcl.contract.func], and that unrelated rule would otherwise fire on the same
lines and obscure which defect is being shown.

## Our Fix

Treat the contract-assertion scope as one a capture may reach through, so the
enclosing function's parameters are capturable from a lambda written in a
predicate.

Tests: `gcc/testsuite/g++.dg/contracts/cpp26/lambda-capture-in-contract.C`,
which checks each case **by the value the predicate saw** rather than by
whether it compiles -- a lambda that captured the wrong entity, or read an
uninitialised closure field, would still compile -- and
`lambda-capture-in-contract-error.C`, which pins that a parameter named with
no capture-default and no capture is still an error.

## Notes

This bug also **masks** part of [GCC-18](gcc-18-lambda-in-predicate-global-not-constified.md)
on stock: whether a by-reference capture of a local or parameter is
const-qualified inside a predicate cannot even be measured there, because the
capture is rejected before the constification question arises. GCC-18's
reproducer therefore exercises the namespace-scope half, which is reachable
on stock. Anyone fixing either should check the other.

Found by the 2026-09-05 audit comparing every plain-`-fcontracts` test in our
suite against stock trunk.
