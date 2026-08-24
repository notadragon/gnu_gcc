// { dg-options "-fcontracts-p3850 -D__STDC_WANT_ASSERT_USES_CONTRACTS__" }
// { dg-do run { target c++26 } }

// P3290 <cassert> integration, library side.
//
// A *failing* assert always aborts: there is no way to select an evaluation
// semantic for a cassert violation.  It is reported to the handler as
// enforce, and -fcontract-evaluation-semantic= does not apply to it -- that
// flag governs contract assertions.  So the violation path needs
// dg-shouldfail, which this testsuite does not use anywhere; it is covered
// from g++.dg (p3290-assert-h.C and friends).  What is checked here is the
// part a library test can observe: with the want-macro defined, <cassert>
// selects the contracts integration rather than the platform assert, and a
// satisfied assert stays silent.
//
// The feature-test macro is the piece that silently disappears if
// bits/version.h is regenerated without an __cpp_lib_assert_can_use_contracts
// entry in version.def, which is why it is asserted here rather than only in
// feature_test_macros.cc.

#include <cassert>
#include <contracts>
#include <testsuite_hooks.h>

#ifndef __cpp_lib_assert_can_use_contracts
# error "__cpp_lib_assert_can_use_contracts not defined via <cassert>"
#endif

static int calls = 0;

void handle_contract_violation(const std::contracts::contract_violation& v)
{
  ++calls;
}

int main()
{
  int x = 1;
  assert(x == 1);
  // A satisfied assert must not reach the handler.
  VERIFY( calls == 0 );
}
