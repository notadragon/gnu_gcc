# GCC-610: A lambda capture named in an outlined contract crashes the compiler

**Status:** Fixed here (commit [934ddf891091](https://github.com/notadragon/gnu_gcc/commit/934ddf891091ae4a327615927d36c697515632be))
**Resolved by:** `1260-outlined-lambda-capture-proxy`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`
**Upstream Link:** None found (searched 2026-09-26, on
`contract-checks-outlined`, `contract lambda capture outlined` and
`gimplify_var_or_parm_decl contract`).  Related: PR126038 (GCC-16 here), which
crashes at the same assertion for the same source without
`-fcontract-checks-outlined`; see the report for why this is a separate
defect.
**Affects:** measured 2026-09-26 -- reproduces on 16.2.0 and trunk
17.0.0 20260922; fixed on this branch.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] ICE in gimplify_var_or_parm_decl for a lambda capture named in a contract under -fcontract-checks-outlined` |

Attachments:

| File | Description |
|---|---|
| [`outlined-lambda-capture-ice.cpp`](outlined-lambda-capture-ice.cpp) | Explicit capture named in a precondition, checks outlined: ICE |

````
A contract on a lambda that names one of the lambda's captures crashes the
compiler when the checks are outlined.

```
int main ()
{
  int k = 5;
  auto lam = [k] (const int x) pre (x != k) { return x; };
  return lam (1) - 1;
}
```

```
$ g++ -std=c++26 -fcontracts -fcontract-checks-outlined -c t.cpp
t.cpp: In member function 'constexpr void main()::<lambda(int)>::operator().pre(int)':
t.cpp:4:39: internal compiler error: in gimplify_var_or_parm_decl, at gimplify.cc:3298
    4 |   auto lam = [k] (const int x) pre (x != k) { return x; };
      |                                     ~~^~~~
```

The same happens for a by-reference capture, for a postcondition, and for
a capture named alongside a postcondition's own captures.

This is related to, but not the same defect as, PR126038, which crashes at
the same assertion for this source without -fcontract-checks-outlined.  A
fix for PR126038 alone leaves this one: with it applied, the inline check
compiles and runs correctly and the outlined one still crashes, for the
reason below.


ANALYSIS

The predicate names the capture through its capture proxy, a local VAR_DECL
whose DECL_VALUE_EXPR is `__closure->k`.  To emit an outlined check,
remap_and_emit_conditions copies the contract and remap_contract
(gcc/cp/contracts.cc) rewrites the predicate onto the outlined function's
parameters with copy_tree_body_r.  That walk copies the proxy, because it is
local to the lambda's operator(), and copy_node keeps the DECL_VALUE_EXPR --
which still names the __closure PARM_DECL of operator(), not the one of
operator().pre.  Gimplifying the outlined function then meets a parameter of
another function.

Mapping each capture proxy in the predicate to its DECL_VALUE_EXPR, itself
walked with the same copy_body_data so that __closure is remapped, fixes it.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       ICE
  compiler-explorer   16.2.0                        yes (gimplify.cc:3426)
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes (gimplify.cc:3298)
  local build -g      17.0.0 20260922, 7aa4b1c5054  yes (gimplify.cc:3298)
````

## Reproducer

See [`outlined-lambda-capture-ice.cpp`](outlined-lambda-capture-ice.cpp) in
this directory.

## Our Fix

`gcc/cp/contracts.cc`, `remap_contract`: for a lambda, collect the capture
proxies in the predicate and insert each into the decl map as its value
expression, remapped with the same `copy_body_data`.  Regression test:
`g++.dg/contracts/cpp26/contract-lambda-capture-outlined-run.C`.

## Notes

Found 2026-09-26 while fixing a crash in outlined postcondition captures on
lambdas (our P3098 work), whose run test reached this with an explicit
capture.  Not yet filed; consider filing it together with the pending
PR126038 comment (GCC-16), since a maintainer fixing that one will meet this
next under -fcontract-checks-outlined.
