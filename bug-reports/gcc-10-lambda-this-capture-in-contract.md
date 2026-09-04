# GCC-10: Contract predicate on a `this`-capturing lambda reads the raw closure object

**Status:** Fixed here (commit `cbccf256658`)
**Component:** c++ / contracts
**Upstream Link:** —

## Bug Report

A contract predicate on a lambda that captured `this` reads the raw closure
object as if it were the enclosing class, because `remap_dummy_this_1`
rewrites any tree for which `is_this_parameter` holds, including the
lambda's captured-`this` proxy. This is silent wrong code (not a crash), and
can appear to pass or fail depending on stack contents.

## Reproducer

See [`gcc-10-lambda-this-capture-in-contract.cpp`](gcc-10-lambda-this-capture-in-contract.cpp)
in this directory.

## Our Fix

`gcc/cp/contracts.cc`: restrict the `remap_dummy_this_1` rewrite to
`PARM_DECL`s only, leaving the proxy `VAR_DECL`'s existing
`DECL_VALUE_EXPR` to resolve correctly.

## Notes

Silent-miscompile class of bug; worth flagging clearly as such in any
upstream filing since it will not show up as a crash or diagnostic.
