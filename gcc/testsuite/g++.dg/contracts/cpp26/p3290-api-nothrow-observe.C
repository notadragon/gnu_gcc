// P3290: nothrow observe overload invokes handler with correct fields.
// This TU does NOT enable -fcontracts-p4298, so the nothrow_t overload binds
// to the plain std::contracts variant and reports classic observe.  (D4298's
// noexcept_observe is reported only when the caller sets -fcontracts-p4298;
// see p4298-p3290-nothrow.C for that case.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <new>
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
  if (!v.comment() || std::strcmp(v.comment(), "nothrow observe") != 0)
    __builtin_abort();
}

int main() {
  std::contracts::handle_observed_contract_violation(
      std::nothrow, "nothrow observe");
  if (handler_count != 1)
    __builtin_abort();
}
