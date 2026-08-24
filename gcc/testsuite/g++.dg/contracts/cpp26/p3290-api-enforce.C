// P3290: handle_enforced_contract_violation invokes handler then terminates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "contract violation" }

#include <contracts>
#include <cstring>

static bool handler_called = false;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  handler_called = true;
  if (v.kind() != std::contracts::assertion_kind::manual)
    __builtin_abort();
  if (v.semantic() != std::contracts::evaluation_semantic::enforce)
    __builtin_abort();
  if (v.detection_mode() != std::contracts::detection_mode::unspecified)
    __builtin_abort();
  if (!v.comment() || std::strcmp(v.comment(), "test comment") != 0)
    __builtin_abort();
}

int main() {
  std::contracts::handle_enforced_contract_violation("test comment");
  __builtin_abort();
}
