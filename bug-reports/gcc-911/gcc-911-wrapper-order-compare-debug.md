# GCC-911: -g changes the order of the caller-side contract wrappers, so -fcompare-debug fails

**Status:** Open (upstream); fixed here
**Resolved by:** [7fc5019a3981](https://github.com/notadragon/gnu_gcc/commit/7fc5019a3981b2ffa279fe17dbb9eef98d2b1a94) (the uid hash) and [2bcd6b6193ba](https://github.com/notadragon/gnu_gcc/commit/2bcd6b6193ba24de51cd876776bb99ffb998d89f) (which rewrites `decl_wrapper_fn` with it)
**Component:** c++
**Keywords (ours -- upstream sets its own):** `compare-debug-failure`
**Upstream Link:** [PR128021](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=128021)
(filed 2026-10-20 by someone else, with a candidate patch; found by the
Phase 5.5 Bugzilla sweep of 2026-10-24)
**Affects:** measured 2026-10-24 -- stock trunk `17.0.0 20261025`
(`cd7a85774a9`): the reporter's three-function program fails
`-fcompare-debug` in 5/10 runs at `-O0`, 5/10 at `-O1`, 2/10 at `-O2` and
10/10 at `-Og` (the order depends on tree addresses, so it varies per run);
the 48-callee reproducer here fails 20/20 at `-O0`, `-Og` and `-O2`.  This
branch: 0/10 at each of `-O0`, `-Og`, `-O1`, `-O2` with the caller-side
checks enabled by a configuration file; with the reporter's flags alone
once `-fcontracts-client-check=all` selected caller-side checks (GCC-912,
fixed 2026-10-25), as `g++.dg/contracts/cpp26/pr128021.C` does.
**Verify case:** `GCC-911` (compile, the reporter's flags at `-Og`).

## Reproducer

[`gcc-911-wrapper-order-compare-debug.cpp`](gcc-911-wrapper-order-compare-debug.cpp):
48 functions, each with a precondition, all called from `main`, compiled
with `-fcontracts -fcontracts-client-check=all -Og -g -fcompare-debug`.
The reporter's program is the same with three functions.

## Why

`emit_contract_wrapper_func` defines the caller-side wrappers by traversing
`decl_wrapper_fn`, a `hash_map` keyed by the callee's `FUNCTION_DECL` and
hashed by address upstream.  `-g` allocates more trees, so the keys'
addresses, the traversal order, and with it the order of the wrappers (and,
optimizing, of the `.Lcontract_violationN` objects) differ between the two
compilations `-fcompare-debug` makes.

## Fixed here

The contracts tables on this branch hash a declaration by its uid
(`contract_pch_hash`, from GCC-646/GCC-647's fix for precompiled headers),
including `decl_wrapper_fn`, so the traversal order no longer depends on
where trees are allocated.  (It is uid-hash order, not declaration order;
the PR's candidate patch emits in insertion order instead.)  Regression
test: `g++.dg/contracts/cpp26/pr128021.C` (24 callees, `-Og`, caller-side
checks enabled by `callerside-checks/callerside-enforce-caller.json`).

Nothing has been posted to the PR; any comment is the user's.
