# GCC-646: a function template from a precompiled header loses its contracts when instantiated after it

**Status:** Open (upstream); fixed here
**Resolved by:** [7fc5019a3981](https://github.com/notadragon/gnu_gcc/commit/7fc5019a3981b2ffa279fe17dbb9eef98d2b1a94)
**Component:** c++ (contracts, PCH)
**Keywords (ours -- upstream sets its own):** `wrong-code`, `accepts-invalid`
**Upstream Link:** None found (searched 2026-10-06: Bugzilla quicksearch for
`contracts pch`, `contract precompiled header`, `pch contract template`).
**Affects:** measured 2026-10-06: this branch (5 of 5 `.gch` builds) and
stock trunk (`17.0.0 20261005`, 2 of 3), both wrong; the result depends on
the particular `.gch` (a rebuilt one is right about 4-13% of the time), and
a given `.gch` behaves the same on every use.
**Verify case:** none -- `verify.sh` has no PCH mode, and the measurement
would move between runs with the `.gch`.

## Reproducer

[`gcc-646-pch-template-contracts.h`](gcc-646-pch-template-contracts.h) and
[`gcc-646-pch-template-contracts.cpp`](gcc-646-pch-template-contracts.cpp):

    // header, precompiled
    template <class T> constexpr T ct (T x) pre (x > 0) post (r: r < 9) { return x; }
    // TU
    #include "gcc-646-pch-template-contracts.h"
    static_assert (ct (0) == 0);     // accepted

Without the `.gch` the precondition violation is diagnosed.  At run time
under `observe` the instance's checks are missing too (its assembly
differs from the build without the PCH).  Non-template functions, members,
constructors and lambdas declared in the header keep theirs.

## Why (unconfirmed)

The contract specifiers live in `contract_decl_map` (`cp/contracts.cc`), a
GC hash map keyed on tree pointers; after a PCH reload the template's entry
is presumably found only when the reloaded addresses happen to hash the
same.  The PCH-build dependence fits that.

## Found by

Mirroring the EDG fork's `pch/constexpr` and `pch/runtime` tests,
2026-10-06.  The watch test was flaky by
nature (`pch.exp` rebuilds the `.gch` per option set); the regression test
`g++.dg/pch/contract-template-after-pch.C` checks twelve templates instead.  Clang is right
(`clang/test/Contracts/pch-contracts-constexpr.cpp`).

## Fixed here

Confirmed: `contract_decl_map` hashed its keys by address, writing a precompiled header moves every tree, and the table is not rehashed, so after the reload a lookup found the entry only by chance.  The map, and the other contracts tables keyed by declarations, types or identifiers, now hash a decl or type by its uid and an identifier by its spelling (`contract_pch_hash`), which a PCH preserves.  Regression tests: `g++.dg/pch/contract-template-after-pch.C` (twelve templates, so that a regression cannot pass by chance) and the templates added to `g++.dg/pch/contract-runtime.C`.
