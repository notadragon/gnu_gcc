// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4301 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
#include <cstdio>
void handle_contract_violation(const std::contracts::contract_violation& v) {
  std::printf(v.report() == nullptr ? "report=null\n" : "report=nonnull\n");
}
int f(int x) { contract_assert(x > 0); return x; }
int main() { f(0); }
// { dg-output "report=null" }
