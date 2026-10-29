# GCC-574: Constructor-precondition member rule: unevaluated uses rejected, qualified-ids not checked

**Kind:** defect (rejects valid + accepts invalid)
**Status:** Fixed here (commit [8bbcde847947](https://github.com/notadragon/gnu_gcc/commit/8bbcde847947d826be592cbc520a2289b9b6c812)) -- found
2026-09-25 (review round #12 fan-out, c-b-4; ruled in DECISIONS.md I1)
**Resolved by:** `0220-ctor-pre-qualified-member` (qualified half) and
`1200-xobj-member-in-predicate` (unevaluated half, in the same tests)
**Component:** c++
**Keywords (ours -- upstream sets its own):** `rejects-valid`, `accepts-invalid`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `contract sizeof member constructor`, `constructor
precondition sizeof`, `required when accessing a member within a constructor
precondition`, plus every open bug with `contract` in its summary; related
but different: PR127294 (the same diagnostic fired in an explicit-object
member function).  Earlier search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`), identically; fixed on this branch
**Workaround:** name the member through `this->` inside the unevaluated
operand (`sizeof(this->b)`); nothing needed for the accepts-invalid half
**Clang:** also rejects the unevaluated uses (CLANG-566 in that fork, and
additionally `requires { b; }`, which GCC accepts); correct on the qualified
half (rejects `pre(C::b)` and `post(E::b)`)

## Ruling

DECISIONS.md I1 (2026-09-25):

> **Answer:** the example is correct, within an unevaluated operand this use
> should be allowed.  Make the assumption that the restriction on the
> transformation applies only when E is potentially evaluated, as that is
> what the core issue will suggest.

and, from the same entry: "Either way GCC must reject `pre(C::b)`."

The report says openly that the normative text and the example disagree on
the unevaluated half, so a maintainer who reads the wording literally is
not surprised; the qualified half does not depend on that question.
DECISIONS.md K1 (2026-09-26): submit the complete report, both halves and
the caveat; whether to raise a core issue too is decided at submission.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] constructor precondition: sizeof(member) rejected (the standard's example), C::member accepted` |

Attachments:

| File | Description |
|---|---|
| [`ctor-pre-unevaluated-member-rejected.cpp`](ctor-pre-unevaluated-member-rejected.cpp) | The [expr.prim.id.general] example and other unevaluated operands (rejected) |
| [`ctor-pre-qualified-member-accepted.cpp`](ctor-pre-qualified-member-accepted.cpp) | A potentially evaluated qualified-id naming a member (accepted) |

````
[expr.prim.id.general]/4 transforms an id-expression that names a
non-static member into a class member access through (*this), and makes
that ill-formed in a constructor's precondition or a destructor's
postcondition.  GCC gets both edges of that rule wrong.

1. Unevaluated operands are rejected.  The example in that paragraph marks

  C() pre(sizeof(b) > 0);   // OK, b is not potentially evaluated.

GCC rejects that example, and every other unevaluated use:

```
struct C {
  bool b;
  int f() const;
  C(int) pre(sizeof(b) > 0);          // u1: OK, the standard's example
  C(float) pre(sizeof(f()) > 0);      // u2: OK, unevaluated
  C(double) pre(noexcept(f()));       // u3: OK, unevaluated
  C(unsigned) pre(requires { b; });   // OK, accepted
  C(long) pre(&this->b);              // OK (example), accepted
};
struct D {
  bool b;
  ~D() post(sizeof(b) > 0);           // u4: OK, unevaluated
};
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only \
    ctor-pre-unevaluated-member-rejected.cpp
ctor-pre-unevaluated-member-rejected.cpp: In constructor 'C::C(int)':
ctor-pre-unevaluated-member-rejected.cpp:15:21: error: 'C::b' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   15 |   C(int) pre(sizeof(b) > 0);          // u1: OK, the standard's example
      |                     ^
ctor-pre-unevaluated-member-rejected.cpp: In constructor 'C::C(float)':
ctor-pre-unevaluated-member-rejected.cpp:16:23: error: 'int C::f() const' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   16 |   C(float) pre(sizeof(f()) > 0);      // u2: OK, unevaluated
      |                       ^
ctor-pre-unevaluated-member-rejected.cpp: In constructor 'C::C(double)':
ctor-pre-unevaluated-member-rejected.cpp:17:26: error: 'int C::f() const' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   17 |   C(double) pre(noexcept(f()));       // u3: OK, unevaluated
      |                          ^
ctor-pre-unevaluated-member-rejected.cpp: In destructor 'D::~D()':
ctor-pre-unevaluated-member-rejected.cpp:23:20: error: 'D::b' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   23 |   ~D() post(sizeof(b) > 0);           // u4: OK, unevaluated
      |                    ^
```

A caveat on this half: read literally, the normative sentence applies the
transformation to an id-expression that is not potentially evaluated too
when the member belongs to the current class, which contradicts the
example.  We take the example as the intent -- the point of the rule is
that no object is within its lifetime, which an unevaluated operand does
not need -- and expect a core issue to resolve it that way.  GCC is at
least inconsistent as it stands: requires { b; } is accepted.

2. A qualified-id is not checked.  A potentially evaluated C::b is
transformed in exactly the same way (it is not the operand of unary &), so
it is ill-formed, but GCC accepts it:

```
struct C {
  bool b;
  C(char) pre(C::b);                  // q1: error expected, accepted
};
struct E {
  bool b;
  ~E() post(E::b);                    // q2: error expected, accepted
};
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only \
    ctor-pre-qualified-member-accepted.cpp
$
```

Expected: the first file compiles cleanly, and the second is rejected on
q1 and q2 as pre(b) would be.


DISCOVERY

Found while reviewing an implementation of C++26 contracts against the
working draft.


ANALYSIS

From reading the code, not confirmed in a debugger: in
finish_id_expression_1 (gcc/cp/semantics.cc), the FIELD_DECL branch and the
member-function-set branch both issue the diagnostic when
processing_contract_condition and contract_class_ptr == current_class_ptr,
without looking at cp_unevaluated_operand; the qualified-id (scope) branch
has no check at all and goes straight to finish_qualified_id_expr.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       u1-u4 rejected  q1-q2 accepted
  compiler-explorer   16.1.0                        yes             yes
  compiler-explorer   16.2.0                        yes             yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes             yes
````

## Our Fix

`gcc/cp/semantics.cc`, `finish_id_expression_1`: the `FIELD_DECL` and
member-function-set tests skip an unevaluated operand
(`!cp_unevaluated_operand`), and the qualified-id branch applies the same
test to a data member or non-static member function of the class or a
base, except when the name forms a pointer to member.  Regression tests:
`g++.dg/contracts/cpp26/ctor-pre-unevaluated-member.C` and
`ctor-pre-qualified-member.C`.
