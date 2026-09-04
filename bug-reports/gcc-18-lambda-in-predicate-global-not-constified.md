# GCC-18: Global named inside a lambda within a predicate is not constified

**Status:** Fixed here (commit `5442adee87a`)
**Component:** c++ / contracts
**Upstream Link:** —

## Bug Report

GCC constifies a namespace-scope variable named directly in a predicate, but
not the same variable named inside a lambda written within that predicate —
violating [expr.prim.id.unqual]/3's own P2900 example. This is the mirror
image of an already-fixed Clang bug, where Clang under-constified only
automatic-storage variables the same way.

## Reproducer

See [`gcc-18-lambda-in-predicate-global-not-constified.cpp`](gcc-18-lambda-in-predicate-global-not-constified.cpp)
in this directory.

## Our Fix

`gcc/cp/*`: the constification check now looks for an *enclosing* contract
scope rather than only the innermost binding level, while still exempting
lambda-capture proxies and predicate-local declarations.

## Notes

Mirror image of an already-fixed Clang bug in the `llvm_llvm-project` fork,
where Clang under-constified only automatic-storage variables in the same
way; worth cross-referencing when filed.
