// Verify that "constexpr" key in JSON config parses without error and that
// runtime behavior is still determined by the runtime entry (observe).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/constexpr-config-parse.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

void f(int x) pre(x > 0) { }

int main() {
  // Runtime semantic is "ignore" (constexpr:false entry or catch-all ignore).
  // No handler call; violations stays 0.
  f(-1);
  if (violations != 0)
    __builtin_abort ();
  return 0;
}
