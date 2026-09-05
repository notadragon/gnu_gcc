# GCC-31: A contract-assertion scope is not treated as a capture-transparent scope

**Status:** Fixed here -- layer 1 in `e3c949476e4`, layer 2 in the commit
that added this paragraph (`git log -- gcc/cp/semantics.cc`)
**Component:** c++ / contracts, lambdas
**Upstream Link:** [PR117435](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117435) -- **already filed, and it reports
LAYER 2 as the primary symptom.** The reporter's ICE is
`expand_expr_real_1, at expr.cc:11415` (the same assertion as ours, the line
number having moved since), and their analysis reads "It would seem that x is
not captured". Filed 2024, using the old `[[pre: ]]` attribute spelling, in
which the capture parses and then fails in the middle end. Independent
confirmation that the two layers are one bug. Correlated 2026-09-05.
**Affects:** measured 2026-09-05 -- reproduces on stock g++ trunk with plain
`-fcontracts`

## Bug Report

A lambda written in a contract predicate is a different function from the one
the contract belongs to, so naming the enclosing function's parameter in it is
a **capture**. GCC does not treat the contract-assertion scope that way, and
the consequence has two layers -- the second only observable once the first is
fixed, which is why they belong in one report.

### Layer 1: the capture is rejected outright

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

### Layer 2: with the capture accepted, a nested assert bypasses it

```cpp
void f (int x) pre ([x] { contract_assert (x >= 0); return x > 0; } ()) { }
```

```
internal compiler error: in expand_expr_real_1, at expr.cc:11792
```

whose assertion reads "Variables inherited from containing functions should
have been lowered by this point".

A `contract_assert` nested inside the predicate lambda re-enters the
contract-condition state, and that state **exempts** a parameter reference
from the capture machinery -- correctly so for a predicate written directly on
the function, where the contract is part of that function's declaration and
there is nothing in between to capture. Inside a lambda there is, and the
exemption leaves a bare reference to the *enclosing* function's parameter in
the lambda body. It survives to expansion and trips the assertion above.

Stock g++ cannot exhibit this today because layer 1 rejects the capture first.
**A fix that stops at layer 1 hands users a construct whose codegen has never
run**, so the reproducer covers both and anyone fixing this should check the
second before declaring it done.

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
predicate (`e3c949476e4`), and stop the contract-condition exemption from
applying inside such a lambda, so a nested `contract_assert` captures like any
other reference rather than naming the outer parameter directly. Both changes
are in `finish_id_expression_1`; the second moves the contract-condition
exemption out of the guard and into the `else`, so it is reached only after
the in-predicate-lambda case has been taken.

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
