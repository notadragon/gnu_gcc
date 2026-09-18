// P3290: source_location is correctly populated.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>
#include <source_location>

static unsigned last_line = 0;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_line = v.location().line();
}

int main() {
  unsigned expected_line = __LINE__ + 1;
  std::contracts::handle_observed_contract_violation("here");
  if (last_line != expected_line)
    __builtin_abort();

  auto loc = std::source_location::current();
  std::contracts::handle_observed_contract_violation("there", loc);
  if (last_line != loc.line())
    __builtin_abort();
}
