# GCC-910: a VLA captured by reference in a lambda in a function template crashes the compiler at C++26

**Status:** Open (upstream); fixed here
**Resolved by:** [cd4981120f7a](https://github.com/notadragon/gnu_gcc/commit/cd4981120f7a5e343ab146732e7d774e228a5eeb)
**Component:** c++
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`
**Upstream Link:** [PR127911](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127911)
(filed 2026-10-15 by someone else; found by the Phase 5.5 Bugzilla sweep of
2026-10-24)
**Affects:** measured 2026-10-24 -- stock trunk `17.0.0 20261025`
(`cd7a85774a9`, checking enabled) fails the assertion in
`set_contract_capture_flag` at `-std=c++26` and is clean at `-std=c++17` and
`-std=gnu++20`; 16.2.0 segfaults at `-std=c++26`; 16.1.0 and 15.3.0 are
clean at `-std=c++17`.  This branch is clean (release install, built at the
2026-10-24 rebase; not measured on a checking build -- the look-through
reaches the closure member through the VLA proxy's `COMPONENT_REF`
chain, so the checking assertions it keeps are satisfied, by reading).
**Verify case:** `GCC-910` (syntax, `-std=c++26`).

## Reproducer

[`gcc-910-vla-capture-template-ice.cpp`](gcc-910-vla-capture-template-ice.cpp),
the reporter's program:

    template <typename T> void foo (T n) {
      intptr_t buf[n];
      [&buf]() { __typeof(buf) baz; }();
    }
    void bar () { foo (42); }

No contract appears.  C++26 turns contracts on by default, which is what
reaches the contracts code.

## Why

When the lambda is instantiated, `process_outer_var_ref` finds the VLA's
existing capture proxy and, because contracts are on and the use is not in a
contract predicate, clears the proxy's contract-only mark with
`set_contract_capture_flag (d, false)`.  Upstream's function asserts that
the proxy's value expression is a `COMPONENT_REF` of the closure's field; a
VLA proxy's value expression instead rebuilds the array from the
{pointer, bound} capture struct (`build_capture_proxy`), so the assertion
fails (and a release compiler reads the wrong operand and crashes).

This is [GCC-495](../gcc-495/gcc-495-contract-assert-vla-implicit-capture-ice.md)'s
defect, reached from valid code without any contract: GCC-495 reaches the
same function from a `contract_assert` naming a VLA.

## Fixed here

By GCC-495's fix: `set_contract_capture_flag` looks through a VLA proxy's
value expression (and a `*this` proxy's `ADDR_EXPR`) to the closure member.
Regression test: `g++.dg/contracts/cpp26/pr127911.C` (the reporter's
program, a VLA read through a by-reference capture, and a capture-default
form).

Nothing has been posted to the PR; any comment is the user's.
