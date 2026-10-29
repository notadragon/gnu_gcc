// P3100: signed integer overflow, quick_enforce -- traps on overflow.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-overflow-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "signed overflow quick_enforce traps" }

#include <contracts>
#include <cstdlib>
#include <climits>

// quick_enforce never calls the handler: a call is a failure, and
// exiting normally is how a dg-shouldfail test reports one.
void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

__attribute__((noinline)) int add (int a, int b) { return a + b; }

int main () { return add (INT_MAX, 1); }
