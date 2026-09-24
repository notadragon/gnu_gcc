// P3100: the same gap as p3100-null-ref-and-call.C, for the alignment
// reaction.  ubsan_maybe_instrument_reference_or_call hard-coded
// IMPLICIT_UB_NONE into operands 6/7/8 as well as 3/4/5, so a routed
// alignment check was missed on a reference binding while the equivalent
// member access was instrumented.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-align-ref.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct alignas (8) A { long long x; };

A &
ref_mis (A *p)
{
  A &r = *p;
  return r;
}

long long
load_mis (A *p)
{
  return p->x;
}

// { dg-final { scan-tree-dump-times "__builtin_trap" 2 "optimized" } }
