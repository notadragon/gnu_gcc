// P3100: when a throwing divide-by-zero handler unwinds, the destructors of
// in-scope automatic variables must run (the guard's handler call is a proper
// unwinding call).  observe semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-by-zero-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E {};
struct S { static int dtors; ~S () { ++dtors; } };
int S::dtors = 0;

void handle_contract_violation (const std::contracts::contract_violation&) {
  throw E{};
}

int main () {
  int x = 1, y = 0;   // runtime divisor
  bool caught = false;
  try {
    S s;
    int i = x / y;    // observe handler throws -> unwind must destroy s
    (void) i;
  } catch (E&) {
    caught = true;
  }
  if (!caught) __builtin_abort ();
  if (S::dtors != 1) __builtin_abort ();
}
