// P3290: nothrow overloads terminate if handler throws.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "terminates" }

#include <contracts>
#include <new>

void handle_contract_violation(const std::contracts::contract_violation&) {
  throw 42;
}

int main() {
  std::contracts::handle_observed_contract_violation(std::nothrow, "throws");
  __builtin_abort();
}
