# GCC-30: A coroutine's postcondition may odr-use a const by-value parameter

**Status:** Fixed here (commit `5dfac566c71`)
**Component:** c++ / contracts, coroutines
**Upstream Link:** UNKNOWN -- not yet filed
**Affects:** measured 2026-09-05 -- reproduces on stock g++ trunk with plain
`-fcontracts`

## Bug Report

```cpp
Task by_value (const int x) post (x > 0) { co_return; }   // accepted
```

The standard states the consequence outright, as a note on
[dcl.fct.def.coroutine]:

> An odr-use of a non-reference parameter in a postcondition assertion of a
> coroutine is ill-formed.

The two rules that produce it are [dcl.contract.func], which requires a
parameter a postcondition odr-uses to be `const`, and
[dcl.fct.def.coroutine]/5, which direct-initializes the coroutine's parameter
copy from an xvalue of the **unqualified** type -- which cannot be formed from
a `const` parameter. Both cannot hold at once, so there is no valid spelling
of a by-value parameter here.

GCC catches only one half. The non-`const` spelling is rejected, but by the
ordinary const rule, which is not coroutine-aware; write the parameter `const`
to satisfy that rule and the program is accepted. The measurement is what
makes the shape clear:

| | stock g++ trunk | this branch |
|---|---|---|
| `Task f (int x) post (x > 0)` | rejected (const rule) | rejected, both rules |
| `Task f (const int x) post (x > 0)` | **accepted** | rejected |

So the coroutine restriction is missing outright rather than mis-worded, and
the const rule was masking its absence.

## Reproducer

See [`gcc-30-coroutine-postcondition-const-param.cpp`](gcc-30-coroutine-postcondition-const-param.cpp)
in this directory, with four controls: the non-const spelling, a reference
parameter (fine -- its frame copy binds to the same object), a precondition
(the const rule and so this restriction are postcondition rules), and a
non-coroutine with the same signature.

## Our Fix

Diagnose it once the function is known to be a coroutine, which is only after
its body has been parsed -- and therefore after its contracts. The parameters
carry the "odr-used in a postcondition" flag set when the predicate was
walked, so the check is a walk over them at that later point.

Test: `gcc/testsuite/g++.dg/contracts/cpp26/coroutine-postcondition-param.C`.

## Notes

Found by a deliberate contracts-x-coroutines sweep; surfaced as an untracked
upstream bug by the 2026-09-05 audit comparing every plain-`-fcontracts` test
against stock trunk.

Clang had the same gap and was fixed alongside; contracts are not upstream in
Clang, so there is nothing to file there.
