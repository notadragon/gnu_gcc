// P3595: linkage "C" uses the name verbatim as the symbol (here a mangled C++
// symbol), binding to the same function as the "C++" qualified form.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -O0" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-cname.json" }
#include <contracts>
namespace mylib {
  std::contracts::evaluation_semantic contract_semantic();
}
void f(int x) pre(x > 0) { }
// Require the symbol at a boundary (it is used verbatim under C linkage).
// { dg-final { scan-assembler "\[ \t\]_ZN5mylib17contract_semanticEv\[^A-Za-z0-9_\]" } }
