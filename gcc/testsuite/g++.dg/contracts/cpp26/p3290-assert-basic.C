// P3290: assert macro invokes contract-violation handler with __STDC_WANT_ASSERT_USES_CONTRACTS__.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290 -D__STDC_WANT_ASSERT_USES_CONTRACTS__" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "contract violation" }

#include <cassert>
#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void handle_contract_violation(const std::contracts::contract_violation& v) {
  std::fputs ("contract handler ran\n", stderr);
  if (v.kind() != std::contracts::assertion_kind::cassert)
    std::_Exit (0);
  if (v.semantic() != std::contracts::evaluation_semantic::enforce)
    std::_Exit (0);
  if (v.detection_mode() != std::contracts::detection_mode::predicate_false)
    std::_Exit (0);
  if (!v.comment() || std::strcmp(v.comment(), "1 == 2") != 0)
    std::_Exit (0);
}

int main() {
  assert(1 == 1);
  assert(1 == 2);
  std::_Exit (0);
}

// A failed check exits normally, which dg-shouldfail reports as a failure;
// the handler must have run.
// { dg-output "contract handler ran" }
