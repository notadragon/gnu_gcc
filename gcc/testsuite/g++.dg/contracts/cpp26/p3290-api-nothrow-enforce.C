// P3290: nothrow enforce overload invokes handler with correct fields.
// This TU does NOT enable -fcontracts-p4298, so the nothrow_t overload binds
// to the plain std::contracts variant and reports classic enforce.  (D4298's
// noexcept_enforce is reported only when the caller sets -fcontracts-p4298;
// see p4298-p3290-nothrow.C for that case.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "contract violation" }

#include <contracts>
#include <new>
#include <cstring>

void handle_contract_violation(const std::contracts::contract_violation& v) {
  if (v.kind() != std::contracts::assertion_kind::manual)
    __builtin_abort();
  if (v.semantic() != std::contracts::evaluation_semantic::enforce)
    __builtin_abort();
  if (v.detection_mode() != std::contracts::detection_mode::unspecified)
    __builtin_abort();
  if (!v.comment() || std::strcmp(v.comment(), "nothrow enforce") != 0)
    __builtin_abort();
}

int main() {
  std::contracts::handle_enforced_contract_violation(
      std::nothrow, "nothrow enforce");
  __builtin_abort();
}
