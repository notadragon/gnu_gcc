# GCC-31: A contract-assertion scope is not treated as a capture-transparent scope

**Status:** Fixed here -- both layers in
[9da30bfb2887](https://github.com/notadragon/gnu_gcc/commit/9da30bfb2887ea84d7366186a2a5b777e27a47bb)
**Resolved by:** `1190-lambda-capture-in-predicate` -- both layers are edits to
`finish_id_expression_1`, which that commit owns
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
predicate, and stop the contract-condition exemption from
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

This bug also **masks** part of [GCC-18](../gcc-18/gcc-18-lambda-in-predicate-global-not-constified.md)
on stock: whether a by-reference capture of a local or parameter is
const-qualified inside a predicate cannot even be measured there, because the
capture is rejected before the constification question arises. GCC-18's
reproducer therefore exercises the namespace-scope half, which is reachable
on stock. Anyone fixing either should check the other.

Found by the 2026-09-05 audit comparing every plain-`-fcontracts` test in our
suite against stock trunk.

## Pending upstream comment (PR117435)

*Drafted for the user to post or drop; nothing here has been sent.  Checked
2026-09-25: no comment from berne@notadragon.com on PR117435 or PR117436.
Every claim re-measured 2026-09-25 against stock `17.0.0 20260922` (the
Compiler Explorer nightly), by the value the predicate saw rather than by
whether it compiles, except the second-half ICE, which stock cannot reach
(layer 1 rejects first) and which is taken from this writeup's Layer 2
measurement.  The older draft (in
`p3850impl/final-passes/RESUME-lambda-capture-in-contracts.md`) opened with
"this is not GCC-only"; that was about our Clang fork, whose contracts are
not upstream, so it is dropped.  Its "out-of-line member definition" row
is dropped too: that shape is a declaration adding contracts, which is
ill-formed on its own account.  Attach [9da30bfb2887](https://github.com/notadragon/gnu_gcc/commit/9da30bfb2887ea84d7366186a2a5b777e27a47bb)
and its two tests; it depends on [266658dff3a0](https://github.com/notadragon/gnu_gcc/commit/266658dff3a0d901c91c7e15718c9618c83723cf), which
introduces the flag marking a lambda as being in a contract predicate.*

~~~text
A C++26-syntax version of this, since comment 3 notes the attribute form
is gone, and a fix.

[expr.prim.lambda.capture]/3.3 allows a capture-default or simple-capture
in a lambda whose innermost enclosing scope is a contract-assertion scope,
so these are well-formed:

  int g_seen = -1;
  bool record (int v) { g_seen = v; return true; }

  void g (int x) pre ([x] { return record (x); } ()) {}   // rejected
  struct S { void f (int x) pre ([x] { return record (x); } ()) {} };  // OK

On trunk (17.0.0 20260922) the free function gives "use of parameter
outside function body" for every capture form -- [x], [&x], [&] and [=].
The member function works for all of them, and so does a lambda's own
precondition; in both, the predicate sees 7 after a call with 7.

That is the split to expect.  A member's contract is parsed once the class
is complete, when the parameters already belong to the function.  A free
function's contract is parsed straight off the declarator, before there is
a FUNCTION_DECL, so DECL_CONTEXT of the parameter is still null;
outer_var_p requires DECL_FUNCTION_SCOPE_P, so the name never reaches
process_outer_var_ref, and finish_id_expression_1 reports it as a use
outside the function body.  process_outer_var_ref itself copes with the
null context -- the enclosing non-lambda function context is null too, so
the two compare equal -- so the first half of the fix is to call it for a
lambda that is inside a contract predicate instead of erroring.

The second half is the ICE this report was filed for, which the rejection
currently hides.  With only the first half applied, a contract_assert
inside the predicate lambda:

  void f (int x) pre ([x] { contract_assert (x >= 0); return x > 0; } ()) {}

  internal compiler error: in expand_expr_real_1

sets processing_contract_condition again, and that exempts the parameter
from the capture machinery -- right for a predicate written directly on
the function, wrong inside a lambda, which is a different function.  The
bare reference to the enclosing function's parameter survives to
expansion.  The fix tests that exemption only after the in-predicate-lambda
case.

The report's own testcase, which has no capture-default, stays ill-formed
and is now diagnosed as "'x' is not captured" rather than as a use outside
the function body.

PR117436, the member-function companion, no longer reproduces: with a
[this] capture a contract on a member function reads the right member on
trunk.  I suggest closing it.
~~~

## Pending upstream comment (PR117436)

*Same status and checks as the PR117435 comment above.  This PR is not a
GCC-31 bug, and it has no row of its own because it does not reproduce;
it is here only because the PR117435 comment above recommends closing it.*

~~~text
This no longer reproduces with the C++26 syntax (the attribute form is no
longer accepted, as noted in PR117435 comment 3).  On trunk (17.0.0
20260922):

  #include <cstdio>
  int g_seen = -1;
  bool record (int v) { g_seen = v; return true; }

  struct f {
    int x = 3;
    void f2 () pre ([this] { return record (x); } ()) {}
  };

  int main () { f o; o.f2 (); std::printf ("%d\n", g_seen); }

compiles and prints 3.  Suggest closing.
~~~
