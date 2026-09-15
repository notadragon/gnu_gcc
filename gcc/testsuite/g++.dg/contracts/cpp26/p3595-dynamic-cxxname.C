// P3595: linkage "C++" with a qualified name mangles to the namespaced symbol.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -O0" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-cxxname.json" }
#include <contracts>
namespace mylib {
  std::contracts::evaluation_semantic contract_semantic();
}
void f(int x) pre(x > 0) { }
// The generated call must reference the mangled namespaced symbol at a symbol
// boundary (not as a substring of some other re-mangled name).
// { dg-final { scan-assembler "\[ \t\]_ZN5mylib17contract_semanticEv\[^A-Za-z0-9_\]" } }
