// P3100: a THROWING contract-violation handler at flow-off-end
// ({stmt.return.flow.off}).  In a non-noexcept function the handler-thrown
// exception propagates out of the function, under both observe and enforce
// (D3100R8 Option A: implicit-assertion sites are potentially-throwing).  Here
// each call is in a try/catch, so the exception is caught by the caller.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-throw-nonnoexcept.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E { int tag; };
static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation&) {
  ++calls;
  throw E{7};
}

namespace obs_ns { int f (int x) { if (x > 0) return x; } }   // observe
namespace enf_ns { int f (int x) { if (x > 0) return x; } }   // enforce

int main () {
  int caught = 0;
  try { obs_ns::f (-1); } catch (E& e) { if (e.tag == 7) ++caught; }
  try { enf_ns::f (-1); } catch (E& e) { if (e.tag == 7) ++caught; }
  if (caught != 2) __builtin_abort ();   // both propagated and were caught
  if (calls != 2) __builtin_abort ();    // handler ran once per call
}
