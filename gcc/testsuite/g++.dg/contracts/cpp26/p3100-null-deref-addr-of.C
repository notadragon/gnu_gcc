// P3100: &*p does not access the object, so the .access leaf of the
// null-dereference check does not fire there even under quick_enforce; the
// indirection is the .indirection leaf's (GCC-799,
// p3100-null-unaccessed-indirection.C).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O0 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int *addr_of_deref (int *p) { return &*p; }   // no load/store: not an access

int main ()
{
  int *p = nullptr;
  // &*p folds to p; no access occurs, so .access must not trap here.
  return addr_of_deref (p) == nullptr ? 0 : 1;
}

// { dg-final { scan-tree-dump-not "__builtin_trap" "optimized" } }
