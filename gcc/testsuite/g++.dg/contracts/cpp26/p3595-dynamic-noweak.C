// P3595: with "provideweak": false, the compiler emits no weak default
// definition for the selector.  A strong user-supplied selector must still
// link and drive the dynamic semantic normally.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-noweak.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>

using std::contracts::evaluation_semantic;

int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// Strong definition: the only definition, since provideweak is false.
evaluation_semantic p3595_sel_nw() { return evaluation_semantic::observe; }

void f(const int x) pre(x > 0) { }

int main() {
  f(-1);                         // observe -> handler called, continue
  if (violations != 1) __builtin_abort();
}
