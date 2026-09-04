# GCC-1: Contract on a function with a parameter pack breaks the redeclaration check

**Status:** Fixed here (commit `3092d1ee80a`)
**Component:** c++ / contracts
**Upstream Link:** [PR124395](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124395) (also duplicates PR126837, PR126836)
**Affects:** stock g++ 16.2.0, trunk; originally confirmed via public
Compiler Explorer, 2026-08-20

## Bug Report

A contract specifier on a function with a variadic parameter pack breaks
`check_postconditions_in_redecl`'s lockstep walk of the template's and
instantiation's parameter lists. Depending on the pack shape this either
ICEs, or silently rejects a well-formed program with a diagnostic naming the
wrong parameter. Three distinct pack shapes are covered by the reproducers
below: an empty pack, a pack in the tail position that ICEs, and a pack in
the tail position that instead produces a misattributed diagnostic.

## Reproducer

See [`gcc-01a-contract-pack-empty-ice.cpp`](gcc-01a-contract-pack-empty-ice.cpp),
[`gcc-01b-contract-pack-tail-ice.cpp`](gcc-01b-contract-pack-tail-ice.cpp), and
[`gcc-01c-contract-pack-tail-misattributed.cpp`](gcc-01c-contract-pack-tail-misattributed.cpp)
in this directory.

## Our Fix

`gcc/cp/contracts.cc`: align the parameter run before the first pack from
the front, and the run after the last pack from the back, instead of a
lockstep walk, so the check is correct regardless of what the pack expands
to.

## Notes

This bug matches an existing upstream report (PR124395) and duplicates two
others (PR126837, PR126836), so it is a good candidate for a comment linking
our fix and reproducers rather than a fresh filing.
