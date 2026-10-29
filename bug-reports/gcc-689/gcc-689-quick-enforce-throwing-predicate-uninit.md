# GCC-689: a throwing predicate under quick_enforce draws a bogus -Wmaybe-uninitialized

**Status:** Open (upstream); fixed here
**Resolved by:** [76208014c18f](https://github.com/notadragon/gnu_gcc/commit/76208014c18f720e81e3c9a908704a916cc0b2de)
**Component:** c++ (contracts)
**Keywords (ours -- upstream sets its own):** `diagnostic`
**Upstream Link:** reported in a comment on another PR, not filed on its own:
[PR125163](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125163) comment 2
(2026-05-24).  None found as a separate PR (searched 2026-10-10: Bugzilla
quicksearch for `quick_enforce uninitialized`, `contract
maybe-uninitialized`, `quick_enforce`).
**Affects:** measured 2026-10-10: 16.1.0, 16.2.0, stock trunk
(`17.0.0 20261005`) and this branch (before the fix) warn identically.

**Bugzilla Summary:** [contracts] bogus -Wmaybe-uninitialized for a
potentially-throwing predicate under quick_enforce

## Reproducer

[`gcc-689-quick-enforce-throwing-predicate-uninit.cpp`](gcc-689-quick-enforce-throwing-predicate-uninit.cpp),
at `-std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce
-O2 -Wall -c`:

    bool positive (int);
    void pre_fn (int x) pre (positive (x)) { }
    void assert_fn (int x) { contract_assert (positive (x)); }

    warning: 'D.2679' may be used uninitialized [-Wmaybe-uninitialized]

once per check, at `-O1` and above, for any predicate that is not
`noexcept`; once the function is inlined into a caller the warning repeats
there.  A `noexcept` predicate, or `-fno-contracts-conservative-ipa`, makes
it go away.

## Why

Stock `build_contract_check` (`gcc/cp/contracts.cc`; on this branch
`emit_check_for_semantic`) wraps a potentially-throwing predicate as

    try { tmp = !pred; } catch (...) { reaction (); }
    if (tmp) reaction ();

For observe and enforce the handler also sets `tmp = false` after calling the
violation handler; for quick_enforce it does not, because the reaction is
meant never to return.  But under stock's `-fcontracts-conservative-ipa`
(the default) the quick_enforce reaction is a call to the `noipa` wrapper
`__tu_terminate_wrapper` (on this branch it was `__tu_quick_enforce_wrapper`,
GCC-103), which is not `noreturn`, so the optimizers see the handler fall
through to the read of a `tmp` that was never stored.

On this branch the cause is gone: the refinable-body work removed
`-fcontracts-conservative-ipa` and its wrappers, upstream's
`__tu_terminate_wrapper` (on upstream's code the reaction calls
`std::terminate` directly) and this branch's `__tu_quick_enforce_wrapper`
(GCC-103; the quick_enforce reaction is an inline `__builtin_trap`), so the
reaction is `noreturn` and the handler needs no store.  An earlier fix here
set the temporary to false after the quick_enforce reaction as well; it
was dropped with the wrapper.  The regression test is
`g++.dg/contracts/cpp26/contract-quick-enforce-throwing-predicate-uninit.C`.

## Workaround

On stock, `-fno-contracts-conservative-ipa` (the trap is then emitted inline and is
`noreturn`), or a `noexcept` predicate.

## Found by

Reading PR125163 while recording GCC-152, 2026-10-10.
