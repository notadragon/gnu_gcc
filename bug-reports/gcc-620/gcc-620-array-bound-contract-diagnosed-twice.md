# GCC-620: an array bound's contract violation is diagnosed twice

**Status:** Open (upstream); fixed here
**Resolved by:** the reporting rewrite in [69a440da7a16](https://github.com/notadragon/gnu_gcc/commit/69a440da7a16a11c5e097a7fbcc582f8f565d417), which reports a problem once per assertion, construct and function
**Component:** c++ / constexpr
**Keywords (ours -- upstream sets its own):** `diagnostic`
**Upstream Link:** None found (searched 2026-09-29).
**Affects:** measured 2026-09-29 against stock trunk (`17.0.0 20260922`),
16.2.0 and this branch: identical on all three.

## Reproducer

[`gcc-620-array-bound-contract-diagnosed-twice.cpp`](gcc-620-array-bound-contract-diagnosed-twice.cpp),
at `-std=c++26 -fcontracts`:

    constexpr int f(const int& v) pre(false) { return 1; }

    int use(int p)
    {
      char a[f(p)];
      return sizeof a;
    }

    repro.cpp:1:31: error: contract predicate is false in constant expression
    repro.cpp:1:31: error: contract predicate is false in constant expression

Under `-fcontract-evaluation-semantic=observe` the warning is doubled the
same way.  Initializing a `const int` from the same call reports it once,
which suggests the array-bound path evaluates the bound twice and reports
what each evaluation recorded; not yet confirmed in the source.

## Found by

The `notional` group of the contract test matrix (every cell with an array
bound whose evaluation completes with a contract problem).  The emitted
tests cannot see the repetition -- a DejaGnu `dg-error` matches both copies
-- so `verify.sh` is what tracks it: its digest counts diagnostics.
