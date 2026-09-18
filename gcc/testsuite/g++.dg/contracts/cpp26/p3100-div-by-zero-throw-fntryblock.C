// P3100: divide-by-zero with a THROWING handler where the division sits in a
// function-try-block's try body -- the division site is already inside the try
// scope, so the function's own handler catches the throw (no special compiler
// handling needed, unlike flow-off-end).  observe semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-by-zero-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E {};
static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation&) {
  ++calls;
  throw E{};
}

int divi (int a, int b) try { return a / b; } catch (E&) { return -1; }

int main () {
  if (divi (1, 0) != -1) __builtin_abort ();   // caught by own handler
  if (calls != 1) __builtin_abort ();
  if (divi (10, 2) != 5) __builtin_abort ();   // normal division unaffected
  if (calls != 1) __builtin_abort ();
}
