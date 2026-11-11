# GCC-507: Implicit member access inside a lambda in a constructor precondition is accepted

**Status:** Open (upstream)
**Resolved by:** `1200-xobj-member-in-predicate`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `precondition lambda constructor`, `lambda constructor
precondition member`, the diagnostic text, plus every open bug with
`contract` in its summary; PR117436 is a member named inside a lambda in an
ordinary member function's contract being rejected -- the opposite
direction, not this -- and PR127428 is an unrelated ICE.  Earlier search
2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`) and this branch all accept s1 (review
round #12 fan-out, c-b-22)
**Clang:** correct (rejects s1), though it follows the error with a spurious
"value of type 'void' is not contextually convertible to 'bool'"

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] implicit this->member inside a lambda in a constructor precondition is not diagnosed` |

Attachments:

| File | Description |
|---|---|
| [`ctor-pre-member-in-lambda.cpp`](ctor-pre-member-in-lambda.cpp) | Constructor preconditions naming a member directly, through a lambda, and through explicit `this` |

````
[expr.prim.id.general]/4 makes the implicit (*this). transformation of an
id-expression ill-formed when it occurs "in the predicate of a precondition
assertion of a constructor" (or of a postcondition assertion of a
destructor).  GCC diagnoses the direct case but not the same name inside a
lambda in the predicate, which is still in the predicate.

```
struct S {
  int m;
  S(unsigned) pre([&]{ return m > 0; }());          // s1: error
  S(long) pre([this]{ return this->m > 0; }());      // s2: OK
  S(char) pre([]{ struct L { int q; int get() { return q; } }; return true; }());  // s3: OK
  S(int) pre(m > 0);                                 // control: error, diagnosed
};
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only ctor-pre-member-in-lambda.cpp
ctor-pre-member-in-lambda.cpp: In constructor 'S::S(int)':
ctor-pre-member-in-lambda.cpp:15:14: error: 'S::m' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   15 |   S(int) pre(m > 0);                                 // control: error, diagnosed
      |              ^
```

Expected: the same error for s1 (line 12).  s2 names the member through an
explicit this and s3 names a local class's own member, so both are fine.


DISCOVERY

Found while reviewing an implementation of C++26 contracts against the
working draft.


ANALYSIS

From reading the code, not confirmed in a debugger: in
finish_id_expression_1 (gcc/cp/semantics.cc), FIELD_DECL branch, the check
is contract_class_ptr && contract_class_ptr == current_class_ptr.  Inside
the lambda's call operator current_class_ptr is the closure's, so the test
fails and the member is reached through the implicitly captured this.  The
comparison needs to look through enclosing lambdas to the contracted
function, while leaving a local class's own members alone.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       s1 accepted
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes
````

## Our Fix

`gcc/cp/semantics.cc`, `finish_id_expression_1`: the data-member and
member-function checks also fire inside a lambda in the predicate, when the
member belongs to the constructor's or destructor's class or a base
(`member_via_lambda_in_cdtor_contract_p`, which walks the binding levels to
the contract scope); a local class's own members are left alone.
Regression test: `g++.dg/contracts/cpp26/ctor-pre-member-in-lambda.C`.

## Notes (ours, not for the report)

Fix direction: when `processing_contract_condition` and the name is reached
from a lambda nested in the predicate, walk out of the lambdas to the
contracted function before comparing.

Watch test: `g++.dg/contracts/cpp26/ctor-pre-member-in-lambda.C`
(`dg-error ... { xfail *-*-* }`), mirrored in the Clang fork by the expected
pass `clang/test/Contracts/ctor-pre-member-in-lambda.cpp`.
