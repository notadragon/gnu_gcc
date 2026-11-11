// P3290: handle_enforced_contract_violation invokes handler then terminates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "contract violation" }

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static bool handler_called = false;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  std::fputs ("contract handler ran\n", stderr);
  handler_called = true;
  if (v.kind() != std::contracts::assertion_kind::manual)
    std::_Exit (0);
  if (v.semantic() != std::contracts::evaluation_semantic::enforce)
    std::_Exit (0);
  if (v.detection_mode() != std::contracts::detection_mode::unspecified)
    std::_Exit (0);
  if (!v.comment() || std::strcmp(v.comment(), "test comment") != 0)
    std::_Exit (0);
}

int main() {
  std::contracts::handle_enforced_contract_violation("test comment");
  std::_Exit (0);
}

// A failed check exits normally, which dg-shouldfail reports as a failure;
// the handler must have run.
// { dg-output "contract handler ran" }
