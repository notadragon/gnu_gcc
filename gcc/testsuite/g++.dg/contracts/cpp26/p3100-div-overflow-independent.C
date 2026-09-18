// P3100: the two division UBs at a single site are configured independently --
// divide-by-zero as ignore, and signed-division-overflow as observe -- and each
// fires with its own semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-overflow-independent.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation&) {
  ++calls;
}

int divi (int a, int b) { return a / b; }

int main () {
  // divide-by-zero -> ignore: defined 0, no handler.
  if (divi (10, 0) != 0) __builtin_abort ();
  if (calls != 0) __builtin_abort ();

  // INT_MIN / -1 -> observe: handler runs, then 0.
  if (divi (-__INT_MAX__ - 1, -1) != 0) __builtin_abort ();
  if (calls != 1) __builtin_abort ();

  // ordinary division unaffected.
  if (divi (12, 3) != 4) __builtin_abort ();
  if (calls != 1) __builtin_abort ();
}
