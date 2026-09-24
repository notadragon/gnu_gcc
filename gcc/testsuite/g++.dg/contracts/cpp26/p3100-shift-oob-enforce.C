// P3100: shift-out-of-range enforce -- handler runs, then terminate.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type -Wno-shift-count-overflow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-shift-oob-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "shift-out-of-range enforce terminates" }

#include <contracts>

void handle_contract_violation (const std::contracts::contract_violation&) {}

int shl (int a, int b) { return a << b; }

int main () { return shl (1, 100); }
