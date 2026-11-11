// P3290: nothrow overloads terminate if handler throws.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "terminates" }

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <new>

void handle_contract_violation(const std::contracts::contract_violation&) {
  std::fputs ("contract handler ran\n", stderr);
  throw 42;
}

int main() {
  std::contracts::handle_observed_contract_violation(std::nothrow, "throws");
  std::_Exit (0);
}

// A failed check exits normally, which dg-shouldfail reports as a failure;
// the handler must have run.
// { dg-output "contract handler ran" }
