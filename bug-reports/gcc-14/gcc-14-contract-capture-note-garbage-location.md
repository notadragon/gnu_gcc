# GCC-14: `contract_assert` diagnostic's "declared here" note prints at a garbage location

**Status:** Fixed here (commit [a3dfc0c61b33](https://github.com/notadragon/gnu_gcc/commit/a3dfc0c61b3367de512fce241ff2774a2580603f))
**Resolved by:** `1360-contract-only-capture-after-pruning`
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

## Pending upstream comment (PR126041)

*Drafted for the user to post or drop; nothing here has been sent.  Checked
2026-09-25: no comment from berne@notadragon.com on PR126041.  Every claim
re-measured 2026-09-25 against stock `17.0.0 20260922` (the Compiler
Explorer nightly, which is a checking build) and 16.2.0 (a release build).
The older draft's open question -- whether the trunk ICE is a regression
or something our delta hides -- is settled: the nightly is an independent
stock build, and the ICE is a tree check, so it is this defect caught by
checking rather than a regression.  Attach
[a3dfc0c61b33](https://github.com/notadragon/gnu_gcc/commit/a3dfc0c61b3367de512fce241ff2774a2580603f).*

~~~text
Confirmed, but the trigger is narrower than the description, and the
error itself is correct; only the note's location is broken.

The error is right: under [expr.prim.lambda.closure] a program is
ill-formed if every potential reference to an implicitly captured entity
is inside a contract assertion, since adding a contract must not add a
capture.  Use x in the lambda body as well and the program is accepted.

No structured bindings and no generic lambdas are needed:

  int main () {
    int x = 1;
    auto outer = [&] () {
      auto inner = [&] () { contract_assert (x > 0); };
      inner ();
    };
    outer ();
  }

16.2.0 prints the note at a different garbage line on each run
(-955267790, -954665806 and -60899278 on three consecutive runs), so this
is a read of an unrelated tree, not an arithmetic underflow of a real line
number.  With one level of lambda, or for a name from the immediately
enclosing lambda (factor in comment 1), the location is right.

The trunk nightly (17.0.0 20260922, a checking build) turns it into an
ICE, which shows what is being read:

  internal compiler error: tree check: expected tree that contains 'decl
  minimal' structure, have 'indirect_ref' in cp_parser_lambda_body, at
  cp/parser.cc:13862

With two levels of lambda the captured entity is reached through an
INDIRECT_REF, and DECL_SOURCE_LOCATION is applied to that.  Our fix
recovers the captured variable (strip_normal_capture_proxy /
DECL_CAPTURED_VARIABLE, falling back to cp_expr_loc_or_input_loc) before
taking its location; the note then points at the declaration of x.
~~~
