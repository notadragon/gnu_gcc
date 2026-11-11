# GCC-647: a redeclaration after a precompiled header sees the header's declarations without contracts

**Status:** Open (upstream); fixed here
**Resolved by:** [011c1b11aad2](https://github.com/notadragon/gnu_gcc/commit/011c1b11aad24630b96b69f7913097c12ca052d7)
**Component:** c++ (contracts, PCH)
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-10-06: Bugzilla quicksearch for
`contracts pch`, `contract precompiled header`).
**Affects:** measured 2026-10-06 on this branch: wrong for most `.gch`
builds (right in about 3 of 30).  Stock trunk rejects the repetition too
but words the different-contract case correctly ("mismatched contract
condition").
**Verify case:** `GCC-647` (a setup row precompiles the header, with
address-space randomization off, which makes the stock `.gch` repeatable).
Not on every nightly: on stock trunk `17.0.0 20261110` (`33ed38d4ee09`) the
stock measurement flaps even with randomization off, between the right
answer (only "mismatched contract condition" for B, as here) and "declaration
adds contracts to `int nf2(int)`" (the reloaded table lost the header's
contract), from run to run in no stable proportion.  Upstream's tables are still hashed
by address there, so neither outcome is a fix; read a STOCK move in this row
as noise unless GCC-646b moves too.

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
