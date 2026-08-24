// P3100: a function-try-block on a NOEXCEPT function still catches the
// flow-off throw with its own handler -- the exception never reaches the
// noexcept boundary because the handler catches it before it escapes -- so the
// function returns normally from the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-fntryblock-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E {};
void handle_contract_violation (const std::contracts::contract_violation&) {
  throw E{};
}

int f (int x) noexcept try { if (x > 0) return x; } catch (E&) { return -1; }

int main () {
  if (f (-1) != -1) __builtin_abort ();   // handler caught the throw, returned
  if (f (5) != 5) __builtin_abort ();
}
