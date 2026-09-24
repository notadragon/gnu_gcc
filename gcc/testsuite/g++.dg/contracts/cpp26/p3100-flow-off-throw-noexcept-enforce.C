// P3100: a THROWING handler at flow-off-end in a NOEXCEPT function, enforce
// semantic.  The handler throws; the exception reaches the function's noexcept
// boundary and calls std::terminate.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-throw-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "throwing handler in a noexcept function terminates" }

#include <contracts>

struct E {};
void handle_contract_violation (const std::contracts::contract_violation&) {
  throw E{};
}

int f (int x) noexcept { if (x > 0) return x; }   // enforce; falls off for x<=0

int main () { f (-1); }
