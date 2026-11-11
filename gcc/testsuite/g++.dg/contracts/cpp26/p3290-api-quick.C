// P3290: handle_quick_enforced_contract_violation terminates without handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "terminates" }

#include <contracts>
#include <cstdlib>

void handle_contract_violation(const std::contracts::contract_violation&) {
  /* quick_enforce never calls the handler: a call is a failure, and
     exiting normally is how a dg-shouldfail test reports one.  */
  std::_Exit (0);
}

int main() {
  std::contracts::handle_quick_enforced_contract_violation("quick terminate");
  std::_Exit (0);
}
