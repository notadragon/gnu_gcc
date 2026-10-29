# GCC-648: a parameter pack used only in a lambda's contract assertion is captured implicitly

**Status:** Open (upstream); fixed here
**Resolved by:** [60072d1efe38](https://github.com/notadragon/gnu_gcc/commit/60072d1efe38f36174f3e78ac153adcb3022f3be)
**Component:** c++ (contracts, lambdas)
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** None found (searched 2026-10-06: Bugzilla quicksearch for
`contract_assert lambda capture pack`, `not implicitly captured by a
contract assertion`).  The rule came in with r16 `13e1fc21c9a` (PR124648).
**Affects:** measured 2026-10-06: stock trunk (`17.0.0 20261005`) and this
branch accept line A.

**Bugzilla Summary:** [c++26][contracts] a parameter pack used only in a
lambda's contract_assert is implicitly captured

## Reproducer

[`gcc-648-contract-only-capture-pack.cpp`](gcc-648-contract-only-capture-pack.cpp):

    template <class... Ts> void p (Ts... ts)
    { auto l = [=] { contract_assert (((ts > 0) && ...)); }; l (); }
    template void p (int, int);

Accepted.  An entity odr-used only within contract assertions of a lambda
is not implicitly captured; it must be captured explicitly.  GCC diagnoses
a single parameter (`[=] { contract_assert (t > 0); }`: "'t' is not
implicitly captured by a contract assertion") but not a pack.  Clang
diagnoses both (once per pack element); the EDG front end too
(`sema/contract_only_capture`).

## Found by

The EDG fork's M0-M4 audit, 2026-10-06.  Watch test:
`g++.dg/contracts/cpp26/open-bug-contract-only-capture-pack.C`.

## Fixed here

The contract-only captures were checked only when a lambda was parsed; a pack is captured at instantiation.  The check now runs from finish_lambda_function, which the instantiation reaches too.
