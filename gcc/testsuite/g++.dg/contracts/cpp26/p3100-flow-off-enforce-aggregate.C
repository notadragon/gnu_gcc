// P3100: flow-off-end under enforce catches the bug (calls the handler, then
// terminates) regardless of the return type -- here an aggregate / sret return.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-throw-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "flow-off enforce terminates for an aggregate return too" }

#include <contracts>

void handle_contract_violation (const std::contracts::contract_violation&) {}

struct Big { int a[8]; };
Big f (int x) { if (x > 0) return Big{}; }   // falls off for x <= 0

int main () { Big b = f (-1); (void) b; return 0; }
