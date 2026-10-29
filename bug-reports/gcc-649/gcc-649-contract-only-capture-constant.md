# GCC-649: a constant read in a lambda's contract assertion is diagnosed as not captured

**Status:** Open (upstream); fixed here
**Resolved by:** [60072d1efe38](https://github.com/notadragon/gnu_gcc/commit/60072d1efe38f36174f3e78ac153adcb3022f3be)
**Component:** c++ (contracts, lambdas)
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-10-06: Bugzilla quicksearch for
`not implicitly captured by a contract assertion`, `contract implicit
capture constant`).
**Affects:** measured 2026-10-06: stock trunk (`17.0.0 20261005`) and this
branch reject lines A and B.

**Bugzilla Summary:** [c++26][contracts] "not implicitly captured by a
contract assertion" for a constant that is not odr-used

## Reproducer

[`gcc-649-contract-only-capture-constant.cpp`](gcc-649-contract-only-capture-constant.cpp):

    void f () {
      const int n = 5;
      auto l1 = [=] { contract_assert (n > 0); };  // error
      auto l3 = [] { contract_assert (n > 0); };   // accepted
    }

`n` is usable in constant expressions and only its value is read, so it is
not odr-used and no capture is involved; the contract-only-capture rule
applies to entities that would be implicitly captured.  The same for a
`constexpr` variable, by-reference default, and a lambda's own
precondition.  Clang and the EDG front end accept all of them.

## Found by

The EDG fork's M0-M4 audit, 2026-10-06.  Watch test:
`g++.dg/contracts/cpp26/open-bug-contract-only-capture-constant.C`.

## Fixed here

The check ran before prune_lambda_captures dropped the provisional captures of constants that were not odr-used.  It now runs after it.  A lambda in a template is never pruned, so there a constant's capture is not diagnosed (GCC-653).
