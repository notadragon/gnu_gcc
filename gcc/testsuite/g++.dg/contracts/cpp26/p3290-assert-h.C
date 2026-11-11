// P3290: the assert integration works when <assert.h> is included (not just
// <cassert>).  The contract-violation handler is invoked with kind=cassert
// (proved via dg-output), and a returning handler terminates via std::abort()
// rather than std::terminate() (proved via the set_terminate sentinel: if
// terminate() ran, my_terminate would _Exit(0) and dg-shouldfail would flag it).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290 -D__STDC_WANT_ASSERT_USES_CONTRACTS__" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "assert via <assert.h> aborts" }
// { dg-output "HANDLER kind=5.*(\n|\r\n|\r)" }

#include <assert.h>
#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <exception>

#ifndef __cpp_lib_assert_can_use_contracts
#error "__cpp_lib_assert_can_use_contracts not defined via <assert.h>"
#endif

void handle_contract_violation(const std::contracts::contract_violation& v) {
  // kind=5 is assertion_kind::cassert; proves the contract integration (not the
  // plain C assert) is active through <assert.h>.
  std::fprintf(stderr, "HANDLER kind=%d\n", static_cast<int>(v.kind()));
}

[[noreturn]] void my_terminate() { std::_Exit(0); }

int main() {
  std::set_terminate(my_terminate);
  assert(1 == 2);
  return 0;
}
