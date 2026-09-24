// P3100: a THROWING handler at flow-off-end, with the offending call inside a
// try { } catch (...) { } block: the handler-thrown exception is caught there
// and normal execution resumes after the catch.  observe semantic,
// non-noexcept function.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-throw-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E {};
static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation&) {
  ++calls;
  throw E{};
}

int classify (int x) { if (x > 0) return x * 2; }   // observe; falls off for x<=0

int main () {
  int steps = 0;
  try {
    classify (-1);   // flow-off -> observe handler throws -> caught below
    steps += 100;    // not reached
  } catch (E&) {
    steps += 1;
  }
  steps += 10;       // execution resumes here
  if (steps != 11) __builtin_abort ();
  if (calls != 1) __builtin_abort ();
}
