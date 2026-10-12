# GCC-521: Redeclaration contract predicates are compared after constant folding

**Status:** Fixed here (commit [de4406f666d1](https://github.com/notadragon/gnu_gcc/commit/de4406f666d1d7d4590b3c5427907d39dcb5bb63))
**Resolved by:** `1000-p2900-fixes`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `mismatched contract`, `contract redeclaration fold`,
plus every open bug with `contract` in its summary; nothing about folding.
Earlier search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0, stock trunk
`17.0.0 20260922` (`f008f03eff8`) and this branch all accept both
redeclarations
**Clang:** correct -- "function redeclaration differs in contract specifier
sequence ... contract previously specified with a non-equivalent condition"
**Found:** 2026-09-25, review round #12 fan-out (g-a-6)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] redeclaration contract conditions are compared after constant folding` |

Attachments:

| File | Description |
|---|---|
| [`redecl-condition-folded.cpp`](redecl-condition-folded.cpp) | Two redeclarations whose predicates differ, but not after folding |

````
[dcl.contract.func] requires the contracts on two declarations of a
function to be the same, which for the predicates means they would satisfy
the one-definition rule, i.e. consist of the same token sequence (up to
renaming of parameters).  GCC compares the conditions after constant
folding, so these are accepted:

```
constexpr int z = 0;
void f (int x) pre (x > 0);
void g (int x) pre (x > 0);

void f (int x) pre (x > 1 - 1) {}
void g (int x) pre (x > z) {}
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only redecl-condition-folded.cpp
$
```

With x > 1 on the redeclaration of f instead, GCC reports

  error: mismatched contract condition in declaration

Expected: that error for both f and g.


DISCOVERY

Found while reviewing an implementation of C++26 contracts against the
working draft.


ANALYSIS

mismatched_contracts_p (gcc/cp/contracts.cc) applies cp_fully_fold_init to
both conditions before comparing them with cp_tree_equal (under
comparing_contracts).  Comparing the unfolded conditions would give the
token-sequence comparison the one-definition rule asks for.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       accepted
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes
````

## Our Fix

`gcc/cp/contracts.cc`, `mismatched_contracts_p`: the conditions are compared
as written (`cp_tree_equal` under `comparing_contracts`), not after
`cp_fully_fold_init`.  Folding had also hidden that a postcondition with
captures was emitted as the original contract node, whose condition
genericization then rewrote in place; `apply_postconditions` now emits an
unshared copy.  Regression tests:
`g++.dg/contracts/cpp26/redecl-condition-folded.C`,
and `redecl-capture-after-definition.C` still passes.

## Notes (ours, not for the report)

The file's header comment describes comparing folded trees as the design,
so a patch should update that too.  GCC-520 is the same mistake for P3400
labels (branch-only).

Watch test: `g++.dg/contracts/cpp26/redecl-condition-folded.C`
(xfail dg-error per line).  Clang mirror (expected pass):
`clang/test/Contracts/redecl-condition-folded.cpp`.
