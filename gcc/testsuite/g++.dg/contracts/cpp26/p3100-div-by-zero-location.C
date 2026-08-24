// P3100: the implicit divide-by-zero semantic is resolved *per site* and honors
// source line ranges (config location matching), consistently with every other
// implicit UB check.  Two divisions in the SAME namespace on different lines
// resolve differently, so only source location can distinguish them.
//
// Config (p3100-div-by-zero-location.json), first match wins:
//   1. line 23 of this file -> ignore   (defined 0, no handler)
//   2. default                  -> observe  (handler runs, then defined 0)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-div-by-zero-location.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation (const cs::contract_violation&) { ++calls; }

namespace same_ns {
  // Same namespace, different lines: only the line range distinguishes them.
  int div_ignored (int a, int b) { return a / b; }   // line 23: ignore
  int div_observed (int a, int b) { return a / b; }   // line 24: observe
}

int main () {
  // Line-matched -> ignore: defined 0, no handler call.
  if (same_ns::div_ignored (10, 0) != 0) __builtin_abort ();
  if (calls != 0) __builtin_abort ();

  // Out of range -> observe: handler runs, then defined 0.
  if (same_ns::div_observed (10, 0) != 0) __builtin_abort ();
  if (calls != 1) __builtin_abort ();
}
