# GCC-14: `contract_assert` diagnostic's "declared here" note prints at a garbage location

**Status:** Fixed here (commit `1d9b11d7f12`)
**Component:** c++ / diagnostics
**Upstream Link:** [PR126041](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126041)
**Affects:** stock g++ 16.2.0 (garbage note location); trunk (17.0.0
20260901, the same defect now surfaces as an ICE under tree-checking)

## Bug Report

A `contract_assert` nested inside two levels of lambda, naming an
enclosing-function local, is correctly rejected, but the accompanying
"declared here" note prints at a garbage source location because
`DECL_SOURCE_LOCATION` is applied to an `INDIRECT_REF` rather than the
underlying declaration. On tree-checking builds this is now an ICE instead
of a garbage location. Note for whoever comments on PR126041: its own
description misidentifies the trigger -- structured bindings and generic
lambdas are not required; a plain local and two ordinary nested lambdas
reproduce it identically -- and current trunk turns it into an ICE.

## Reproducer

See [`gcc-14-contract-capture-note-garbage-location.cpp`](gcc-14-contract-capture-note-garbage-location.cpp)
in this directory.

## Our Fix

`gcc/cp/parser.cc`: recover the underlying captured variable (via
`strip_normal_capture_proxy` / `DECL_CAPTURED_VARIABLE`, falling back to
`cp_expr_loc_or_input_loc`) before calling `DECL_SOURCE_LOCATION` for the
note.

## Notes

Matches upstream PR126041, but that report's own description of the trigger
is wrong (it claims structured bindings and generic lambdas are required;
they are not) -- worth correcting in any comment we leave there. Current
trunk has turned the garbage location into an outright ICE on
tree-checking builds.
