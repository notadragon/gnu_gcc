# GCC-503: Implicit captures used only in contract assertions: `this` and nested-lambda captures accepted, generic lambda ICEs

**Status:** Fixed here (commit [cd4981120f7a](https://github.com/notadragon/gnu_gcc/commit/cd4981120f7a5e343ab146732e7d774e228a5eeb))
**Resolved by:** `1270-contract-only-capture-detection`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `accepts-invalid`,
`ice-on-invalid-code`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on the diagnostic text, `contract lambda this capture`,
`contract generic lambda` and `contract_assert lambda this`; PR117435,
PR124648, PR126038, PR126041, PR127283 and PR127287 are other
lambda-in-contract bugs, none of them this.  Earlier search 2026-09-25, same
result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`) and this branch accept CASE=1 and CASE=2;
on CASE=3 all give the error and then crash (a segfault on the checking
trunk build, "confused by earlier errors, bailing out" on the release
builds) (review round #12 fan-out, c-b-5)
**Clang:** diagnoses all three ("'this' is used only in contract assertions
and must be captured explicitly", "local entity 'y' is ..."); for the
generic lambda it says "reference to local variable 't' declared in enclosing
function 'h'" instead of the contract-capture wording

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] implicit lambda captures used only in contract assertions: this and a nested lambda's captures not diagnosed; ICE for a generic lambda` |

Attachments:

| File | Description |
|---|---|
| [`lambda-contract-only-capture.cpp`](lambda-contract-only-capture.cpp) | Three cases selected with `-DCASE=1..3` |

````
[expr.prim.lambda.capture] makes a program ill-formed when all references
to a local entity implicitly captured by a lambda are in the lambda's
contract assertions.  GCC diagnoses the plain case ("'x' is not implicitly
captured by a contract assertion") but misses or mishandles these three.

```
#if CASE == 1
struct S {
  int m;
  void g() {
    auto s1 = [&] pre(m > 0) {};                 // error
    auto s2 = [&] { contract_assert(m > 0); };   // error
  }
};
#elif CASE == 2
void h() {
  bool y = true;
  auto n1 = [=] pre([=]{ return y; }()) {};      // error
}
#elif CASE == 3
void h(int t) {
  auto t1 = [=](auto u) pre(u > t) {};           // error, then ICE
  t1(1);
}
#endif
```

CASE=1: *this is a local entity ([basic.pre]), captured here only for the
contract assertions.  Accepted without a diagnostic.

CASE=2: this is f7 in the example in [expr.prim.lambda.capture] ("error:
outer capture of y is invalid").  Accepted without a diagnostic.  The
testsuite already carries this example:
g++.dg/contracts/cpp26/expr.prim.lambda.closure.p10.C has the f7 line with
"// TODO :" and no dg-error, so once this is fixed that line should expect
the error.

CASE=3: a generic lambda.  The error is issued, and then the compiler
crashes when the lambda is instantiated:

```
$ g++ -std=c++26 -fcontracts -fsyntax-only -DCASE=3 \
    lambda-contract-only-capture.cpp
lambda-contract-only-capture.cpp: In lambda function:
lambda-contract-only-capture.cpp:27:33: error: 't' is not implicitly captured by a contract assertion [-Wtemplate-body]
   27 |   auto t1 = [=](auto u) pre(u > t) {};           // error, then ICE
      |                                 ^
lambda-contract-only-capture.cpp:26:12: note: 't' declared here
   26 | void h(int t) {
      |        ~~~~^
lambda-contract-only-capture.cpp: In instantiation of 'h(int)::<lambda(auto:1)> [with auto:1 = int]':
lambda-contract-only-capture.cpp:28:5:   required from here
   28 |   t1(1);
      |   ~~^~~
lambda-contract-only-capture.cpp:27:33: internal compiler error: Segmentation fault
   27 |   auto t1 = [=](auto u) pre(u > t) {};           // error, then ICE
      |                                 ^
```

That is trunk, built with checking enabled.  16.1.0 and 16.2.0 print the
same error and note and then "lambda-contract-only-capture.cpp:27: confused
by earlier errors, bailing out".  A contract_assert in the body of the same
generic lambda ([&](auto u) { contract_assert(u > t); }) is diagnosed
without crashing.

Expected: an error on every line marked "error", and no crash.


DISCOVERY

Found while reviewing an implementation of C++26 contracts against the
working draft.


ANALYSIS

From reading the code, not confirmed in a debugger: the check is in
cp_parser_lambda_body (gcc/cp/parser.cc), which walks the lambda's own
capture list for FIELD_DECLs with DECL_CONTRACT_CAPTURE_P.  The `this'
capture is evidently never flagged, and in CASE=2 the outer lambda's capture
of y is made on behalf of the inner lambda (which is inside the predicate)
and is not flagged either.  The CASE=3 crash happens at instantiation of the
generic lambda after the error has been issued.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       CASE=1  CASE=2  CASE=3
  compiler-explorer   16.1.0                        accept  accept  error+ICE
  compiler-explorer   16.2.0                        accept  accept  error+ICE
  compiler-explorer   17.0.0 20260922, f008f03eff8  accept  accept  error+ICE
````

## Our Fix

CASE=1: `lambda_expr_this_capture` (`gcc/cp/lambda.cc`) marks an implicit
`this` capture made in a contract predicate as contract-only, and clears the
mark on a later use outside one (`set_contract_capture_flag` is exported and
accepts the capture field).  CASE=2: `add_default_capture`
(`gcc/cp/lambda.cc`), walking the chain of lambdas that capture on behalf of
an inner one, marks a capture as contract-only where the next lambda inward
is inside a contract predicate and this one is not; a later use outside a
predicate clears it, as for a direct capture.  CASE=3: `tsubst_expr`
(`gcc/cp/pt.cc`) no longer copies a pattern capture proxy that the
instantiation lacks after an error.  Regression tests:
`lambda-contract-only-capture.C`, `lambda-contract-only-capture-generic.C`,
`lambda-contract-only-capture-nested.C`; f7 in
`expr.prim.lambda.closure.p10.C` now expects the error (DECISIONS.md K2).

## Notes (ours, not for the report)

The CASE=3 ICE was not reduced further; on a release build the ICE text is
hidden behind "confused by earlier errors".

Mirrored in the Clang fork by `clang/test/Contracts/lambda-contract-only-capture.cpp`.
