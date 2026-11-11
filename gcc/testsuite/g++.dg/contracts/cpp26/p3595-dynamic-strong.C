// P3595: a strong C++ selector overrides the weak default and drives semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-strong.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using std::contracts::evaluation_semantic;

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// Strong definition: return observe so a violation calls the handler+continues.
evaluation_semantic p3595_sel() { return evaluation_semantic::observe; }

void f(int x) pre(x > 0) { }

int main() {
  f(-1);                         // observe -> handler called, continue
  if (violations != 1) __builtin_abort();
  f(1);                          // no violation
  if (violations != 1) __builtin_abort();
}
