# GCC-24: Predicted a MUST_NOT_THROW_EXPR wrapper instead of detecting it

**Status:** Fixed here (commit `5fc7de5d66e`)
**Component:** c++ / contracts, EH-spec interaction
**Upstream Link:** [PR127173](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127173)
**Affects:** stock g++ 16.2.0, trunk (17.0.0 20260901); segfaults on both

## Bug Report

A `noexcept` function carrying a contract segfaults the compiler under
`-fno-enforce-eh-specs`:

```cpp
void f (int x) noexcept pre (x > 1) { }
```

```
$ g++ -std=c++26 -fcontracts -fno-enforce-eh-specs -c repro.C
internal compiler error: Segmentation fault
```

`maybe_apply_function_contracts` assumed a `noexcept` function's body would
be wrapped in a `MUST_NOT_THROW_EXPR`, so it unconditionally took that
wrapper's first operand. The wrapper is produced by `begin_eh_spec_block`,
which `use_eh_spec_block` gates on `flag_enforce_eh_specs` -- so under
`-fno-enforce-eh-specs` there is no wrapper at all, and whatever tree
happened to come first in the body was read as if it were one. A checking
build hits an assertion; a release build (like the two stock binaries
checked here) segfaults. This reproduces on any target under
`-fno-enforce-eh-specs`, not only the original reporter's MIPS
cross-compile -- both stock binaries checked here are ordinary
x86_64-linux-gnu builds and both segfault identically.

## Reproducer

See [`gcc-24-noexcept-body-wrapper-assumption.C`](gcc-24-noexcept-body-wrapper-assumption.C)
in this directory.

## Our Fix

`gcc/cp/contracts.cc`, `maybe_apply_function_contracts`: detect the
`MUST_NOT_THROW_EXPR` wrapper by testing the body for it, rather than
inferring its presence from `TYPE_NOEXCEPT_P`. This also covers the other
reasons `use_eh_spec_block` declines to wrap the body -- a cloned function,
or an implicitly-generated constructor or destructor -- which were latent
behind the same faulty assumption.

## Notes

The reproducer was extracted from the fix commit's own DejaGnu test,
`gcc/testsuite/g++.dg/contracts/cpp26/pr127173.C` (added by gnu_gcc commit
`5fc7de5d66e`), found via `git show 5fc7de5d66e --stat` and read with
`git show 5fc7de5d66e:gcc/testsuite/g++.dg/contracts/cpp26/pr127173.C`.
DejaGnu directive lines were stripped; the C++ source, including the
test's own commentary, was kept as-is.

Measured directly against the stock binaries at
`/home/jberne4/repos/compilers/gcc-16.2.0/bin/g++` and `gcc-trunk/bin/g++`
(17.0.0 20260901), both ordinary release builds (not checking builds):
both segfault identically at the first function in the file (`f`) with
`-std=c++26 -fcontracts -fno-enforce-eh-specs -c`, confirming the release-build
segfault the fix commit describes on both. Older stock compilers (13.4.0,
14.4.0, 15.3.0) do not support C++26 contracts syntax at all and are not
applicable.
