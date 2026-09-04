# GCC-13: Contract-carrying lambda self-reference ICEs on template instantiation

**Status:** Fixed here (commit `5852998dd08`)
**Component:** c++ / contracts
**Upstream Link:** --
**Affects:** stock g++ 16.2.0, trunk (17.0.0 20260901)

## Bug Report

A lambda carrying a contract specifier whose predicate names the lambda's
own parameter or postcondition result ICEs (four distinct crash sites
depending on build/instantiation count) whenever the enclosing template is
instantiated, because `tsubst_function_decl` copies contract specifiers onto
instantiations without substituting them.

## Reproducer

See [`gcc-13-precondition-in-lambda-nested-in-generic-lambda.cpp`](gcc-13-precondition-in-lambda-nested-in-generic-lambda.cpp)
in this directory.

## Our Fix

`gcc/cp/pt.cc`: substitute the lambda's own contract specifiers in
`tsubst_lambda_expr` against the pattern, before the body, via a new shared
`subst_contract_specifiers` helper.

## Notes

Four distinct crash sites were observed depending on build and instantiation
count; the reproducer is written to exercise more than one of them.
