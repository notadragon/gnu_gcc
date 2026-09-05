# GCC-11: Lambda in a postcondition with a result-name-introducer fails to parse

**Status:** Fixed here (commit `bb488b220c0`)
**Component:** c++ / parser
**Upstream Link:** None found. Searched 2026-09-05, including resolved
bugs, on comment text `lambda postcondition result name parse`
**Affects:** stock g++ 16.2.0, trunk (17.0.0 20260901); g++ 15.3.0 predates
the `post` syntax entirely and rejects it outright

## Bug Report

A lambda in a non-member postcondition's predicate fails to parse when the
postcondition has a result-name-introducer (e.g. `post(r : []{...}())`),
because the immediate-contract parse path unconditionally raises
`processing_template_decl`, which lambda parsing cannot handle outside a
real template. This has been broken in every GCC release that ever accepted
`post`.

## Reproducer

See [`gcc-11-lambda-in-postcondition-result-name.cpp`](gcc-11-lambda-in-postcondition-result-name.cpp)
and [`gcc-11-lambda-in-postcondition-testcases.C`](gcc-11-lambda-in-postcondition-testcases.C)
in this directory.

## Our Fix

`gcc/cp/parser.cc` and `gcc/cp/pt.cc`: extend the existing PR99546
lambda-parsing workaround to contract scope, and make `tsubst_lambda_expr`
a no-op for a lambda whose `operator()` already carries no template info.

## Notes

Long-standing since `post` was first accepted; not filed upstream yet.
