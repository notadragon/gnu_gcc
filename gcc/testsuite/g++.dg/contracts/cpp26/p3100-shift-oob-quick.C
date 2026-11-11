// P3100: shift-out-of-range quick_enforce -- traps, no handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-shift-count-overflow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-shift-oob-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "shift-out-of-range quick_enforce traps" }

#include <contracts>
#include <cstdlib>

// quick_enforce never calls the handler: a call is a failure, and
// exiting normally is how a dg-shouldfail test reports one.
void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

int shl (int a, int b) { return a << b; }

int main () { return shl (1, 100); }
