# GCC-24: Predicted a MUST_NOT_THROW_EXPR wrapper instead of detecting it

**Status:** Fixed here (commit [3bf924c07d0f](https://github.com/notadragon/gnu_gcc/commit/3bf924c07d0f001b29a445321728a839f3d271f1))
**Resolved by:** `1220-noexcept-body-wrapper-detected`
**Component:** c++ / contracts, EH-spec interaction
**Upstream Link:** [PR127173](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127173)
**Affects:** stock g++ 16.2.0, trunk (17.0.0 20260901); segfaults on both

## Bug Report

A `noexcept` function with a contract specifier segfaults the compiler under
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
`gcc/testsuite/g++.dg/contracts/cpp26/pr127173.C` (added by [3bf924c07d0f](https://github.com/notadragon/gnu_gcc/commit/3bf924c07d0f001b29a445321728a839f3d271f1)).
DejaGnu directive lines were stripped; the C++ source, including the
test's own commentary, was kept as-is.

Measured directly against the stock binaries at
`gcc-16.2.0/bin/g++` and `gcc-trunk/bin/g++`
(17.0.0 20260901), both ordinary release builds (not checking builds):
both segfault identically at the first function in the file (`f`) with
`-std=c++26 -fcontracts -fno-enforce-eh-specs -c`, confirming the release-build
segfault the fix commit describes on both. Older stock compilers (13.4.0,
14.4.0, 15.3.0) do not support C++26 contracts syntax at all and are not
applicable.

## Pending upstream comment (PR127173)

*Drafted for the user to post or drop; nothing here has been sent.  Checked
2026-09-25: no comment from berne@notadragon.com on PR127173.  Every claim
re-measured 2026-09-25 against stock `17.0.0 20260922` (the Compiler
Explorer nightly) and 16.2.0, both x86_64-linux-gnu.  saaadhu's comment 1
already gives the mechanism, so this adds only the target datum, the
`-fno-exceptions` spelling (found during the re-measurement) and the fix.
The trunk ICE in PR125403 (GCC-23) is at the same assertion.  Attach
[3bf924c07d0f](https://github.com/notadragon/gnu_gcc/commit/3bf924c07d0f001b29a445321728a839f3d271f1).*

~~~text
Not MIPS-specific: it segfaults the same way on x86_64-linux-gnu, with
16.2.0 and with trunk (17.0.0 20260922).  And -fno-exceptions reaches it
too, which is likely to be hit far more often than -fno-enforce-eh-specs:

  void f (int x) noexcept pre (x > 1) { }

  g++ -std=c++26 -fcontracts -fno-exceptions -c ex.C
  internal compiler error: Segmentation fault

That follows from comment 1: use_eh_spec_block also returns false without
flag_exceptions, and for a cloned or a defaulted function, so in each of
those cases there is no
MUST_NOT_THROW_EXPR for maybe_apply_function_contracts to unwrap.

We have a fix that tests the body for the MUST_NOT_THROW_EXPR instead of
inferring it from TYPE_NOEXCEPT_P, and handles the body directly when
there is none.  With it the testcase compiles under both options.
~~~
