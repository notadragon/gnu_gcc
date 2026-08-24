// P3290: handle_quick_enforced_contract_violation terminates without handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "terminates" }

#include <contracts>

void handle_contract_violation(const std::contracts::contract_violation&) {
  __builtin_abort();
}

int main() {
  std::contracts::handle_quick_enforced_contract_violation("quick terminate");
  __builtin_abort();
}
