# GCC-552: An explicit specialization declared with a contract and defined without one loses the contract

**Status:** Fixed here (commit [937b5a592599](https://github.com/notadragon/gnu_gcc/commit/937b5a592599c4e5359d8e8477b31a2231bbf7f0))
**Resolved by:** `1300-explicit-specialization-contracts`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `wrong-code`, `accepts-invalid`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `contract explicit specialization`, plus every open bug
with `contract` in its summary; the only hit, PR125712, is an ICE on an
out-of-line member template, not this.  Earlier search 2026-09-25, same
result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`) and this branch behave identically on both
reproducers.
**Workaround:** repeat the contract on the specialization's definition
**Clang:** drops the contract too (CLANG-553 in that fork), but diagnoses the
mismatch

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] the contract on the first declaration of an explicit specialization is dropped by a definition that omits it, and a differing one is not diagnosed` |

Attachments:

| File | Description |
|---|---|
| [`explicit-spec-contract-dropped.cpp`](explicit-spec-contract-dropped.cpp) | Definition omits the contract: no check |
| [`explicit-spec-contract-mismatch-accepted.cpp`](explicit-spec-contract-mismatch-accepted.cpp) | Two different contracts: accepted |

````
The first declaration of an explicit specialization introduces its
function contract assertions ([dcl.contract.func]), and a later
declaration of it may omit them.  GCC drops them in that case:

```
#include <cstdio>

template <class T> void ft (T x);
template <> void ft<int> (int x) pre (x > 5);
template <> void ft<int> (int x) {}

int main () { ft (1); std::puts ("after"); }
```

```
$ g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe \
    explicit-spec-contract-dropped.cpp && ./a.out
after
```

Expected: a violation of x > 5 before "after".  If the definition repeats
pre (x > 5), the violation is reported.

The two declarations are also never compared, so a definition with a
different contract is accepted:

```
template <class T> void ft (T x);
template <> void ft<int> (int x) pre (x > 5);
template <> void ft<int> (int x) pre (x > 6) {}
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only \
    explicit-spec-contract-mismatch-accepted.cpp
$
```

Expected: "mismatched contract condition in declaration", which is what
GCC reports for the same pair of declarations of an ordinary function.


DISCOVERY

Found while reviewing an implementation of C++26 contracts against the
working draft.


ANALYSIS

Not narrowed down.  An explicit specialization's redeclaration is merged by
register_specialization (gcc/cp/pt.cc) calling duplicate_decls
(gcc/cp/decl.cc); the contract carry-over and comparison that
check_redecl_contract (gcc/cp/contracts.cc) performs for ordinary
redeclarations evidently does not take effect on that path: the omitted
contract is not inherited and the mismatch is not diagnosed.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       dropped  mismatch accepted
  compiler-explorer   16.1.0                        yes      yes
  compiler-explorer   16.2.0                        yes      yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes      yes
````

## Our Fix

`gcc/cp/decl.cc`, `grokfndecl`: attach the contracts before
`check_explicit_specialization` merges the specialization with an earlier
declaration, and do not overwrite a merged declaration's contracts;
`gcc/cp/contracts.cc`, `check_redecl_contract`: a merge with an implicit
instantiation gives the surviving node exactly the specialization's contracts
(none if it declares none), replacing the blanket clearing in
`check_explicit_specialization` (`gcc/cp/pt.cc`); and an explicit
specialization's own redeclarations get the ordinary redeclaration checks, so
one that adds contracts to a first declaration without them is diagnosed
("declaration adds contracts").  Regression tests:
`explicit-spec-contract-dropped.C`, `explicit-spec-contract-mismatch.C`; the
upstream `contracts-tmpl-spec2.C` expected that adding case to be accepted
and now expects the error (DECISIONS.md K4).

## Notes (ours, not for the report)

Fix direction: treat the specialization's first declaration as the one
whose contracts stand, exactly as for ordinary redeclarations.

Watch tests: `g++.dg/contracts/cpp26/open-bug-explicit-spec-contract-dropped.C`
(xfail run) and `open-bug-explicit-spec-contract-mismatch.C` (`dg-error`
xfail); mirrored in the Clang fork
(`clang/test/Contracts/OpenBugs/explicit-spec-contract-dropped.cpp`, XFAIL;
`explicit-spec-contract-mismatch.cpp`, expected pass).
