// P3290: assert macro invokes contract-violation handler with __STDC_WANT_ASSERT_USES_CONTRACTS__.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290 -D__STDC_WANT_ASSERT_USES_CONTRACTS__" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "contract violation" }

#include <cassert>
#include <contracts>
#include <cstring>

void handle_contract_violation(const std::contracts::contract_violation& v) {
  if (v.kind() != std::contracts::assertion_kind::cassert)
    __builtin_abort();
  if (v.semantic() != std::contracts::evaluation_semantic::enforce)
    __builtin_abort();
  if (v.detection_mode() != std::contracts::detection_mode::predicate_false)
    __builtin_abort();
  if (!v.comment() || std::strcmp(v.comment(), "1 == 2") != 0)
    __builtin_abort();
}

int main() {
  assert(1 == 1);
  assert(1 == 2);
  __builtin_abort();
}
