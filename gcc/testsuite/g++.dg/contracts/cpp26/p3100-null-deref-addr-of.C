// P3100: &*p does not access the object, so no null-dereference contract
// assertion fires there even under quick_enforce -- taking the address of a
// dereference is well-defined and must not be instrumented.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O0 -fdump-tree-optimized -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-null-deref-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int *addr_of_deref (int *p) { return &*p; }   // no load/store: not a deref

int main ()
{
  int *p = nullptr;
  // &*p folds to p; no access occurs, so quick_enforce must not trap here.
  return addr_of_deref (p) == nullptr ? 0 : 1;
}

// { dg-final { scan-tree-dump-not "__builtin_trap" "optimized" } }
