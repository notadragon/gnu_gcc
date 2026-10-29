# GCC-606: An explicit by-copy lambda capture in a contract predicate gets the const-ified type

**Kind:** defect (rejects valid)
**Status:** Fixed here (commit [ebd73576d396](https://github.com/notadragon/gnu_gcc/commit/ebd73576d396ea81dc1e09e6b01d3fb2f7c76ca9))
**Found:** 2026-09-25 (review round #12 fan-out, late-2, noted while writing
CLANG-506)
**Resolved by:** `0240-contract-explicit-copy-capture`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `contract mutable capture`, `contract read-only
variable lambda`, `contract closure const capture`, plus every open bug with
`contract` in its summary; related but different: PR117435 (capture of a
parameter in a contract lambda), PR127284 (lambda in a postcondition with a
result name does not parse), PR127293 (namespace-scope variable in a
contract lambda not made const).  Earlier search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`) and this branch reject a1.  On the branch
also a parameter (`pre([x]() mutable { return ++x; }() > 0)`), a result
binding (`post(r: [r]() mutable { return ++r; }() > 0)`) and the same in a
function template; stock cannot reach those shapes because of PR117435 /
PR127284.
**Workaround:** use `[=]` or an init-capture (`[w = v]`)
**Clang:** correct for locals and parameters; rejects `[r]` for a result
binding for another reason (CLANG-506 in that fork)

## Our Fix

`gcc/cp/lambda.cc`, `lambda_capture_field_type`: strip the contract const
wrapper from the initializer of a by-copy capture without an initializer
before taking its type, so the closure member has the entity's own type.  A
by-reference capture names the entity and stays const.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] an explicit by-copy capture in a contract predicate makes the closure member const, so a mutable lambda cannot modify it` |

Attachments:

| File | Description |
|---|---|
| [`contract-explicit-copy-capture-const.cpp`](contract-explicit-copy-capture-const.cpp) | Explicit, implicit and init captures of a local in a `contract_assert` mutable lambda |

````
In a contract predicate an id-expression naming a local variable declared
outside the predicate has const type ([expr.prim.id.unqual]).  That does
not change the variable's type, so a by-copy capture of it is a closure
member of type int, and in a mutable lambda it can be modified.  The
example in [expr.prim.id.unqual] does exactly this with [=] ("++p; // OK,
refers to member of closure type").  GCC gets this right for [=] and for an
init-capture, but not for an explicit simple capture.

```
void a1() {
  int v = 1;
  contract_assert([v]() mutable { return ++v; }() > 0);     // a1: OK, rejected
}
void a2() {
  int v = 1;
  contract_assert([=]() mutable { return ++v; }() > 0);     // a2: OK, accepted
}
void a3() {
  int v = 1;
  contract_assert([w = v]() mutable { return ++w; }() > 0);  // a3: OK, accepted
}
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only \
    contract-explicit-copy-capture-const.cpp
contract-explicit-copy-capture-const.cpp: In lambda function:
contract-explicit-copy-capture-const.cpp:13:44: error: increment of read-only variable 'v'
   13 |   contract_assert([v]() mutable { return ++v; }() > 0);     // a1: OK, rejected
      |                                            ^
```

Expected: all three accepted.


DISCOVERY

Found while reviewing an implementation of C++26 contracts for lambdas in
contract predicates.


ANALYSIS

From reading the code, not confirmed in a debugger:
cp_parser_lambda_introducer (gcc/cp/parser.cc) turns the capture's name
into an id-expression with finish_id_expression, which in a contract
predicate wraps it via constify_contract_access.  add_capture
(gcc/cp/lambda.cc) then computes the field type with
lambda_capture_field_type, whose non-init branch takes unlowered_expr_type
of that wrapper: const int.  build_capture_proxy already strips the wrapper
(strip_contract_const_wrapper) for DECL_CAPTURED_VARIABLE, but by then the
field type has been fixed.  An implicit capture is built from the decl
rather than an id-expression, and an init-capture deduces with auto, which
drops the const, so neither is affected.  Stripping the wrapper before
computing a by-copy field type would fix it (a by-reference capture keeps
its separate const-qualification in add_capture).


VERSIONS -- all on x86_64-linux-gnu

  source              version                       a1 rejected
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes
````

## Notes (ours, not for the report)

Watch test: `g++.dg/contracts/cpp26/open-bug-contract-explicit-copy-capture.C`
(`dg-xfail-if`; local, parameter, result binding and template shapes),
mirrored in the Clang fork by the expected pass
`clang/test/Contracts/OpenBugs/contract-explicit-copy-capture.cpp` (local and
parameter shapes; Clang's result-binding shape is
`result-binding-lambda-explicit-capture.cpp`, XFAIL under CLANG-506).
