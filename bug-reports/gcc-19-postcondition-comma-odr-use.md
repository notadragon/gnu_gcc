# GCC-19: Postcondition wrongly treats the discarded left operand of a comma as odr-using a parameter

**Status:** Fixed here (commit `f3ff6a8e22f`)
**Component:** c++ / contracts
**Upstream Link:** [PR126897](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126897)
**Affects:** measured against this branch's pre-fix baseline only;
stock-version matrix not recorded (matches a pre-existing upstream report,
PR126897, filed independently)

## Bug Report

A postcondition discarding the left operand of a comma expression (e.g.
`post((b, true))`) is wrongly rejected as odr-using the by-value parameter
`b`, requiring it to be const, because the check ran per id-expression (in
`finish_id_expression`) before discardedness of the surrounding expression
could be known. This also affects Clang, but since Clang's contracts
implementation isn't upstream, only the GCC side is filable.

## Reproducer

See [`gcc-19-postcondition-comma-odr-use.C`](gcc-19-postcondition-comma-odr-use.C)
in this directory.

## Our Fix

Moved the [dcl.contract.func]/7 odr-use check from `finish_id_expression`
to a walk over the *finished* predicate that models potential-results (not
subtrees -- `(f(b), true)` still correctly odr-uses `b` as a call argument),
retiring `defer_postcondition_pack_index_check` as a byproduct.

## Notes

The reproducer was extracted from the fix commit's own DejaGnu test,
`gcc/testsuite/g++.dg/contracts/cpp26/pr126897.C` (added by gnu_gcc commit
`f3ff6a8e22f`), found via `git show f3ff6a8e22f --stat` and read with
`git show f3ff6a8e22f:gcc/testsuite/g++.dg/contracts/cpp26/pr126897.C`. The
DejaGnu `dg-do compile` and `dg-additional-options` directive lines were
stripped, along with the trailing `{ dg-error ... }` DejaGnu annotations,
which were replaced with plain `// expected-error "..."` comments so the
lines that are genuinely still expected to diagnose (real odr-uses) remain
marked; the lines specific to this bug (which are wrongly rejected on
unpatched GCC) are called out separately. Matches upstream PR126897; also
affects Clang, but Clang's contracts implementation isn't upstream, so only
the GCC side is filable there.
