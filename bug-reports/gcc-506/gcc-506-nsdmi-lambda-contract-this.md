# GCC-506: A lambda whose precondition names a member, in a default member initializer of a class template, is rejected (ICE on this branch)

**Status:** Fixed here (commit [c72c0e64899d](https://github.com/notadragon/gnu_gcc/commit/c72c0e64899de20a3287f71fcd761fa5bb771009))
**Resolved by:** `4640-contract-substitution-plumbing`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `rejects-valid`;
`ice-on-valid-code` on this branch
**Upstream Link:** [PR127287](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127287) *
+ comment pending.  PR127287, filed by us (GCC-13, "a lambda's own contract
specifiers are never substituted when it is instantiated"), is the same
defect on stock, reached through a predicate
naming the lambda's parameter; this is the same missing substitution reached
through a member named via the captured `this`.  Re-searched 2026-09-26
(Bugzilla REST, summary and comment text, on `contract default member
initializer`, `contract NSDMI`, `contract lambda this capture`,
`dependent_operator_type`, plus every open bug with `contract` in its
summary): nothing else.  So this is a pending comment on PR127287, not a new
bug.
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0 and stock trunk
`17.0.0 20260922` (`f008f03eff8`) reject it (the predicate is left with a
dependent type); this branch ICEs in `tsubst_expr` (review round #12
fan-out, c-b-20)
**Clang:** correct (compiles it and checks the precondition)

## Bug Report

Already filed, by us, as PR127287: stock GCC never substitutes a lambda's
own contract specifiers when the lambda is instantiated as part of a
template.  PR127287 shows it as an ICE in expand_expr_real_1 with a
predicate naming the lambda's parameter, and says a predicate naming only a
global is fine.  This row is the same defect with a predicate that names a
member through the captured `this`, where it shows as a rejection instead;
neither the parameter nor the default member initializer is needed.
Nothing new to file; the useful contribution is the comment below.

| File | Description |
|---|---|
| [`nsdmi-lambda-contract-this.cpp`](nsdmi-lambda-contract-this.cpp) | The shape as found: a default member initializer of a class template |

## Pending upstream comment (PR127287)

*Drafted for the user to post or drop; nothing here has been sent.  Checked
2026-09-26: PR127287 has only its opening comment.  Re-measured 2026-09-26
against 16.1.0, 16.2.0 and stock trunk `17.0.0 20260922` (`f008f03eff8`,
the Compiler Explorer nightly).*

~~~text
The same missing substitution has a second face that is a rejection rather
than an ICE: a lambda in a class template whose contract names a member
through the captured this.  The lambda's parameter is not needed, so the
"names its own parameter" ingredient in comment 0 is sufficient but not
necessary.

  template <class T> struct S {
    T m = 5;
    bool g() { return [this] pre(m > 0) { return true; }(); }
  };
  int main() { return S<int>{}.g() ? 0 : 1; }

  $ g++ -std=c++26 -fcontracts -c t.cpp
  t.cpp: In instantiation of 'bool S<T>::g() [with T = int]':
  ...
  t.cpp:3:34: error: could not convert '(((S<T>*)__closure)->S<T>::m > 0)'
  from ''dependent_operator_type' not supported by dump_type<type error>'
  to 'bool'
  t.cpp:3:34: error: in argument to unary !

The S<T> in the instantiation's diagnostic is the unsubstituted pattern.
With int m instead of T m, 16.2.0 and trunk segfault on that line instead
(internal compiler error: Segmentation fault, at -c); the
same code in a non-template class compiles and runs correctly.  Seen with
16.1.0, 16.2.0 and trunk (17.0.0 20260922).  It was first seen in a default
member initializer, bool r = [this](int x) pre(x > m) { ... }(1);, which
fails the same way.

Note also the (S<T>*)__closure: the predicate reads m through the closure
pointer as if it were the enclosing object, which is PR127283.  A fix that
substitutes the lambda's contract will reach that next for this shape, so
the two probably want fixing together.
~~~

## Our Fix

This branch substitutes a lambda's contracts (`tsubst_lambda_contract_specifiers`, `gcc/cp/pt.cc` -- the missing substitution PR127287 is about).  The default-member-initializer shape still crashed there because the predicate's `this` capture proxy had no local specialization: in a member function, substituting the function's body registers it, but a default member initializer has no body.  `bridge_lambda_capture_proxy_r` now maps the pattern's `this` proxy to the instantiated lambda's.  Regression test: `nsdmi-lambda-contract-this.C`.

## Notes (ours, not for the report)

This branch fixed PR127287's parameter-only shape (GCC-13) in the contract
substitution plumbing, and with a member named through the captured `this`
it now gets further and ICEs: "internal compiler error: in tsubst_expr, at
cp/pt.cc:23578".  Fix direction: substitute the lambda's contract with the
NSDMI's `this` (the closure's captured `this` of the instantiated class) when
the default member initializer is instantiated.

Watch test: `g++.dg/contracts/cpp26/open-bug-nsdmi-lambda-contract-this.C`
(`dg-ice`, matching the branch), mirrored in the Clang fork by the expected
pass `clang/test/Contracts/OpenBugs/nsdmi-lambda-contract-this.cpp`.
