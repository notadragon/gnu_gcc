# GCC-518: A redeclaration that adds or drops a postcondition's result-name-introducer is accepted

**Status:** Fixed here (commit [b6eece227f40](https://github.com/notadragon/gnu_gcc/commit/b6eece227f40af201fe7903ceb32a6fc8cd46985))
**Resolved by:** `1000-p2900-fixes`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `mismatched contract`, `result-name-introducer
redeclaration`, plus every open bug with `contract` in its summary; PR127196
and PR127291 are other redeclaration rules, not this.  Earlier search
2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`) and this branch all accept the
reproducer, in either order
**Clang:** correct -- "function redeclaration differs in contract specifier
sequence ... contract previously specified with result name"
**Found:** 2026-09-25, review round #12 fan-out (g-a-3)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a redeclaration whose postcondition adds or drops the result-name-introducer is accepted` |

Attachments:

| File | Description |
|---|---|
| [`redecl-result-name-presence.cpp`](redecl-result-name-presence.cpp) | The working draft's own example from [dcl.contract.func] |

````
[dcl.contract.func] requires a redeclaration's
function-contract-specifier-seq to be the same as the first
declaration's, and two function-contract-specifiers are the same only if
both specify a result-name-introducer or neither does.  The example in that
subclause reads

```
bool b1;
int g () post (r: b1);
int g () post (b1);             // error: mismatched result-name-introducer presence
```

GCC accepts it without a diagnostic, and the same pair in the opposite
order as well:

```
$ g++ -std=c++26 -fcontracts -fsyntax-only redecl-result-name-presence.cpp
$
```

Expected: an error on the second declaration.


DISCOVERY

Found while reviewing an implementation of C++26 contracts against the
working draft.


ANALYSIS

From reading the code: mismatched_contracts_p (gcc/cp/contracts.cc) compares
the kinds and the (folded) conditions of the two contracts, but never
whether each has a result-name-introducer (POSTCONDITION_IDENTIFIER).


VERSIONS -- all on x86_64-linux-gnu

  source              version                       accepted
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes
````

## Our Fix

`gcc/cp/contracts.cc`, `mismatched_contracts_p`: two postconditions match
only if both have a result-name-introducer or neither does ("mismatched
postcondition result name in declaration"), as the draft's example requires.
Regression test: `g++.dg/contracts/cpp26/redecl-result-name-presence.C`.

## Notes (ours, not for the report)

Fix: add a `POSTCONDITION_IDENTIFIER` presence test to
`mismatched_contracts_p`; on this branch, next to the capture-presence one
(the branch's version also compares P3400 labels and P3098 captures).

Watch test: `g++.dg/contracts/cpp26/redecl-result-name-presence.C`
(xfail dg-error); Clang mirror (expected pass):
`clang/test/Contracts/redecl-result-name-presence.cpp`.
