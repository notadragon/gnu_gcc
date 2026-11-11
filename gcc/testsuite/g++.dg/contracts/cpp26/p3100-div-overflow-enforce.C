// P3100: signed division overflow (INT_MIN / -1) enforce -- handler then
// terminate.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-overflow-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "signed division overflow enforce terminates" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::fputs ("contract handler ran\n", stderr);
}

int divi (int a, int b) { return a / b; }

int main () { return divi (-__INT_MAX__ - 1, -1); }

// The handler runs before termination (a native trap would not print this).
// { dg-output "contract handler ran" }
