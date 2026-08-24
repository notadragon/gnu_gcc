// P3100: signed division overflow (INT_MIN / -1) enforce -- handler then
// terminate.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-overflow-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "signed division overflow enforce terminates" }

#include <contracts>

void handle_contract_violation (const std::contracts::contract_violation&) {}

int divi (int a, int b) { return a / b; }

int main () { return divi (-__INT_MAX__ - 1, -1); }
