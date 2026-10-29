# GCC-647: a redeclaration after a precompiled header sees the header's declarations without contracts

**Status:** Open (upstream); fixed here
**Resolved by:** [7fc5019a3981](https://github.com/notadragon/gnu_gcc/commit/7fc5019a3981b2ffa279fe17dbb9eef98d2b1a94)
**Component:** c++ (contracts, PCH)
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-10-06: Bugzilla quicksearch for
`contracts pch`, `contract precompiled header`).
**Affects:** measured 2026-10-06 on this branch: wrong for most `.gch`
builds (right in about 3 of 30).  Stock trunk rejects the repetition too
but words the different-contract case correctly ("mismatched contract
condition").
**Verify case:** none -- `verify.sh` has no PCH mode, and the measurement
would move between runs with the `.gch`.

## Reproducer

[`gcc-647-pch-redecl-contracts.h`](gcc-647-pch-redecl-contracts.h) and
[`gcc-647-pch-redecl-contracts.cpp`](gcc-647-pch-redecl-contracts.cpp):

    // header, precompiled
    int nf (int x) pre (x > 1);
    int nf (int x);
    int nf2 (int x) pre (x > 1);
    // TU
    int nf (int x) pre (x > 1);   // error: declaration adds contracts
    int nf2 (int x) pre (x > 2);  // error: declaration adds contracts

Repeating the same contracts is valid; the second should be "mismatched
contract condition".  Without the `.gch` that is what happens.

## Why (unconfirmed)

Presumably the same lookup as GCC-646: the first declaration's contracts
are not found in `contract_decl_map` after the reload.

## Found by

Mirroring the EDG fork's `pch/constexpr` tests, 2026-10-06.  Now the regression test
`g++.dg/pch/contract-redecl-after-pch.C`, twelve of each shape (the watch
test was flaky, as GCC-646's).
Clang is right (`clang/test/Contracts/pch-contracts-constexpr.cpp`).

## Fixed here

Same cause and fix as GCC-646: `contract_decl_map` was hashed by address and not rehashed after a PCH reload; it now hashes a decl by its uid.  Regression test: `g++.dg/pch/contract-redecl-after-pch.C` (twelve of each shape).
