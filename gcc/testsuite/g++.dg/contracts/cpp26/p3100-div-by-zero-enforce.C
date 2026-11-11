// P3100: integer divide-by-zero, enforce semantic -- the handler runs, then
// the program terminates (the trapping division is never executed).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-by-zero-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "divide-by-zero enforce terminates" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::fputs ("contract handler ran\n", stderr);
}

int divi (int a, int b) { return a / b; }

int main () { return divi (10, 0); }

// The handler runs before termination (a native trap would not print this).
// { dg-output "contract handler ran" }
