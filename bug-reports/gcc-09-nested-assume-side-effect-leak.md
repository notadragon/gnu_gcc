# GCC-9: Nested `[[assume]]` leaks the inner operand's side effects during constant evaluation

**Status:** Fixed here (commit `eb33a898bab`)
**Component:** c++ / constexpr
**Upstream Link:** --
**Affects:** stock g++ 13.4.0, 14.4.0, 15.3.0, 16.2.0, trunk

## Bug Report

A nested `[[assume]]` leaks the inner operand's side effects during constant
evaluation: `modifiable_tracker`'s destructor unconditionally nulls the
enclosing tracker instead of restoring it. This is not contracts-related; it
reproduces on stock GCC 13.4.0, 14.4.0, 15.3.0, 16.2.0, and trunk.

## Reproducer

See [`gcc-09-nested-assume-side-effect-leak.cpp`](gcc-09-nested-assume-side-effect-leak.cpp)
in this directory.

## Our Fix

`gcc/cp/constexpr.cc`: `~modifiable_tracker` now restores the enclosing
tracker's `modifiable` set instead of nulling it, matching the two
neighboring fields' existing behavior.

## Notes

Purely a core-language constexpr defect, found incidentally while working on
contracts; not filed upstream yet.
