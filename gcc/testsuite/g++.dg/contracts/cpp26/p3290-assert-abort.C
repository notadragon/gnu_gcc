// P3290: a failed assert whose contract-violation handler returns normally
// terminates the program via std::abort(), NOT std::terminate().  The ABI
// terminates enforced violations with abort() so that a user-installed
// std::terminate handler cannot alter contract-termination.
//
// Discrimination: we install a std::terminate handler that would _Exit(0).
// If std::terminate() were used, the program would exit successfully and
// dg-shouldfail would flag the test.  Since assert aborts, the handler never
// runs and the program dies via SIGABRT.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290 -D__STDC_WANT_ASSERT_USES_CONTRACTS__" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "assert aborts rather than terminating" }

#include <cassert>
#include <contracts>
#include <cstdlib>
#include <exception>

void handle_contract_violation(const std::contracts::contract_violation&)
{
  // Returns normally; the ABI must still abort() on completion.
}

[[noreturn]] void my_terminate() { std::_Exit(0); }

int main()
{
  std::set_terminate(my_terminate);
  assert(1 == 2);
  return 0;
}
