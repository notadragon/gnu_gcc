# GCC-16: Contract on a lambda that captures, inside a template, segfaults

**Status:** Fixed here (commit `1f764681499`)
**Component:** c++ / contracts
**Upstream Link:** —

## Bug Report

A contract predicate on a lambda that captures a local (by value or
reference) segfaults when the lambda is written inside an instantiated
template, because nothing maps the pattern lambda's capture proxy to the
instantiation's, and `process_outer_var_ref` has no carve-out for a capture
proxy inside a contract condition. This reproduces on stock g++ 16.2.0 and
g++-trunk (17.0.0 20260901).

## Reproducer

See [`gcc-16-lambda-capture-contract-in-template.C`](gcc-16-lambda-capture-contract-in-template.C)
in this directory.

## Our Fix

`gcc/cp/pt.cc` and `gcc/cp/semantics.cc`: bridge each capture proxy named in
a predicate to the instantiation's, via a two-hop `local_specializations`
lookup registered into the CURRENT map, before substituting contracts;
extend `process_outer_var_ref`'s parameter carve-out to capture proxies.

## Notes

The reproducer was extracted from the fix commit's own DejaGnu test,
`gcc/testsuite/g++.dg/contracts/cpp26/lambda-capture-contract-in-template.C`
(added by gnu_gcc commit `1f764681499`), found via
`git show 1f764681499 --stat` and read with
`git show 1f764681499:gcc/testsuite/g++.dg/contracts/cpp26/lambda-capture-contract-in-template.C`.
The DejaGnu `dg-do run`, `dg-additional-options`, and `dg-skip-if` directive
lines were stripped; the C++ source (including the test's own commentary
comment block) was kept as-is. Requires `-fcontracts
-fcontract-evaluation-semantic=observe` and a hosted `<contracts>` header
(C++26) to build and run.
