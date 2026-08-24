// P3100: null-pointer dereference (ub:expr.unary.dereference.nullptr), quick_enforce
// semantic -- the middle-end null instrumentation emits a trap before the
// dereference, and no violation handler is called.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-null-deref-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "null dereference quick_enforce traps" }

#include <contracts>

int load_it (int *p) { return *p; }

int main ()
{
  int *p = nullptr;
  return load_it (p);
}

// The guarded dereference lowers to a conditional __builtin_trap.
// { dg-final { scan-tree-dump "__builtin_trap" "optimized" } }
