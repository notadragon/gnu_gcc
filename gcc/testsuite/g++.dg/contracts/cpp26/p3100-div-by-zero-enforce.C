// P3100: integer divide-by-zero, enforce semantic -- the handler runs, then
// the program terminates (the trapping division is never executed).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-by-zero-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "divide-by-zero enforce terminates" }

#include <contracts>

void handle_contract_violation (const std::contracts::contract_violation&) {}

int divi (int a, int b) { return a / b; }

int main () { return divi (10, 0); }
