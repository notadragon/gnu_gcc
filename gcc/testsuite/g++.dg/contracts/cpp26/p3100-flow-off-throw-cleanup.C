// P3100: when a throwing flow-off-end handler unwinds, the destructors of
// in-scope automatic variables in the function being fallen off must run.
// observe semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-throw-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E {};
struct S { static int dtors; ~S () { ++dtors; } };
int S::dtors = 0;

void handle_contract_violation (const std::contracts::contract_violation&) {
  throw E{};
}

int f (int x) { S s; if (x > 0) return x; }   // falls off for x <= 0

int main () {
  bool caught = false;
  try { f (-1); } catch (E&) { caught = true; }   // flow-off throws -> unwind s
  if (!caught) __builtin_abort ();
  if (S::dtors != 1) __builtin_abort ();

  // The normal return path destroys s exactly once too.
  S::dtors = 0;
  if (f (5) != 5) __builtin_abort ();
  if (S::dtors != 1) __builtin_abort ();
}
