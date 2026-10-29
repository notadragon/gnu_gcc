# GCC-682: noexcept ignores a throwing default constructor run by an array filler

**Status:** Open (upstream); fixed here
**Resolved by:** [270b8e0a50bd](https://github.com/notadragon/gnu_gcc/commit/270b8e0a50bd5c80394559ccf382909f2dbeb2fd)
**Component:** c++
**Keywords (ours -- upstream sets its own):** `wrong-code`, regression
**Upstream Link:** None found (searched 2026-10-09: `noexcept delete`,
`noexcept array`, `noexcept aggregate array`, `noexcept VEC_INIT_EXPR`,
summaries containing `noexcept` with `array`/`aggregate`/`init`,
`potentially-throwing`, `potentially throwing`, `array filler`,
`is_nothrow_default_constructible`).  Nearest: PR115558 (a trivial
`noexcept(false)` default constructor and value-initialization; a different
path).
**Affects:** measured 2026-10-09 at `-std=c++20`: stock trunk (`17.0.0
20261005`, the CE nightly at `8802cd9fdbe`), 16.2.0, 16.1.0, 15.3.0 and
14.4.0 fail all three assertions; 13.4.0 fails only `A{}`, so `A{{}}` and
`S2{}` are a GCC 14 regression.  This branch fails all three before the fix.
Clang had the same bug (CLANG-664, fixed in the companion fork).

**Bugzilla Summary:** [14/15/16/17 Regression] noexcept of an initializer
list ignores a potentially-throwing default constructor run by its array
filler

## Reproducer

[`gcc-682-noexcept-array-filler.cpp`](gcc-682-noexcept-array-filler.cpp),
at `-std=c++20 -fsyntax-only`:

    struct S { S () noexcept (false); };
    struct A { S s[2]; };
    using S2 = S[2];
    static_assert (!noexcept (A{}), "A{}");
    static_assert (!noexcept (A{{}}), "A{{}}");
    static_assert (!noexcept (S2{}), "S2{}");

All three assertions fail.  The elements of `s` (and of the `S2`) that the
list does not initialize explicitly are copy-initialized from `{}`
([dcl.init.aggr]/5), which calls `S::S()`; that call is an immediate
subexpression of the initialization, so the expression is
potentially-throwing ([except.spec]/6) and `noexcept` must be false.

## Cause

`process_init_constructor_array` (`typeck2.cc`) does not build the
trailing elements' initialization when it is not constant: it sets
`PICFLAG_VEC_INIT`, and `process_init_constructor` wraps the list in a
`VEC_INIT_EXPR` (`build_vec_init_expr`, `tree.cc`) whose element
initialization is computed by `build_vec_init_elt` only to set
`VEC_INIT_EXPR_IS_CONSTEXPR` and then dropped; the constructor calls appear
only when `cp_genericize` expands the `VEC_INIT_EXPR`.  `check_noexcept_r`
(`except.cc`) looks only at `CALL_EXPR` and `AGGR_INIT_EXPR` nodes, so it
never sees `S::S()`.

## Fix here

`check_noexcept_r` also walks, for a `VEC_INIT_EXPR`, the element
initialization `build_vec_init_expr` computed for it, rebuilt by a new
`vec_init_expr_elt_init` (`tree.cc`).  Test:
`g++.dg/cpp0x/noexcept-array-filler.C`.

## Found by

The Clang fork's M17 (CLANG-664): its contract codegen decides whether a
predicate can throw with Sema's equivalent of `noexcept`, which missed the
filler.  GCC's contract codegen does not depend on `noexcept` here, and its
runtime test `predicate-throw-array-filler.C` passes either way.
