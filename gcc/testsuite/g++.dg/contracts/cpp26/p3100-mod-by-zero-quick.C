// P3100: modulo-by-zero parity with divide -- quick_enforce traps.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-by-zero-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "modulo-by-zero quick_enforce traps" }

#include <contracts>
#include <cstdlib>
#include <csignal>

// quick_enforce never calls the handler: a call is a failure, and
// exiting normally is how a dg-shouldfail test reports one.  So is the
// native SIGFPE the unchecked operation would raise.
void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

int m (int a, int b) { return a % b; }

int main ()
{
  std::signal (SIGFPE, [] (int) { std::_Exit (0); });
  return m (10, 0);
}
