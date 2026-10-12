# GCC-633: a generic lambda's precondition naming a capture ICEs in tsubst_expr

**Status:** Open (upstream); fixed here
**Resolved by:** [85b191f9bff7](https://github.com/notadragon/gnu_gcc/commit/85b191f9bff73ee6f94ed432b068d6daa0f88a7a)
**Component:** c++ (contracts, generic lambdas)
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`
**Upstream Link:** None found (searched 2026-10-02: Bugzilla quicksearch for
`tsubst_expr contract lambda`, `lambda pack capture contract`).
**Affects:** measured 2026-10-02 against stock trunk (`17.0.0 20260922`),
16.2.0 and this branch: identical (`cp/pt.cc` 23217 on trunk, 23000 on
16.2, 23654 here).

**Bugzilla Summary:** [c++26][contracts] ICE in tsubst_expr for a generic
lambda whose precondition names a capture

## Reproducer

[`gcc-633-generic-lambda-pre-capture.cpp`](gcc-633-generic-lambda-pre-capture.cpp),
at `-std=c++26 -fcontracts -c`:

    int f () { int t = 1;
               auto l = [t] (auto u) pre (u > t) { return u + t; };
               return l (2); }

    internal compiler error: in tsubst_expr, at cp/pt.cc:23217

The same with `[=]`, and with a trailing return type (`-> int`).  A
non-generic lambda compiles.  Clang (with contracts) accepts it (with a
trailing return type it asserts: CLANG-627 in the Clang fork); the EDG
front end accepts it.

## Found by

The EDG fork's EDG-50 (a generic lambda's own contracts naming its
captures), cross-checked against our GCC and Clang, 2026-10-02.

## Fixed here

A specialization of a generic lambda's operator() has its contracts substituted before its body declares the specialization's capture proxies, so the pattern's proxy had no specialization and fell into tsubst_expr's "late-specified return type" recovery.  The predicate now names the closure member the proxy stands for.
