// P3290: handle_observed_contract_violation invokes handler and returns.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static int handler_count = 0;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  ++handler_count;
  if (v.kind() != std::contracts::assertion_kind::manual)
    __builtin_abort();
  if (v.semantic() != std::contracts::evaluation_semantic::observe)
    __builtin_abort();
  if (v.detection_mode() != std::contracts::detection_mode::unspecified)
    __builtin_abort();
}

int main() {
  std::contracts::handle_observed_contract_violation("first");
  if (handler_count != 1)
    __builtin_abort();
  std::contracts::handle_observed_contract_violation("second");
  if (handler_count != 2)
    __builtin_abort();
}
