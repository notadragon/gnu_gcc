// P3595 dynamic: multi-TU weak-default vs strong-override linking.  The contract
// lives in this TU (a weak selector definition is emitted, returning the config
// default "ignore").  A second TU (p3595-dynamic-multi-tu-2.cc) provides a
// strong definition of the same selector returning "observe".  At link the
// strong definition wins, so the failing precondition is observed (handler
// fires once) instead of ignored.  All prior dynamic tests were single-TU.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-multi-tu.json" }
// { dg-additional-sources "p3595-dynamic-multi-tu-2.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int fired = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++fired;
}

void f(int x) pre(x > 0) { }

int main() {
  f(-1);                       // strong selector -> observe -> fire once
  if (fired != 1) __builtin_abort();
}
