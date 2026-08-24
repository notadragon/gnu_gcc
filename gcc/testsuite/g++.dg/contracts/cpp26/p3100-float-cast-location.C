// P3100: the implicit float-to-integer conversion semantic
// (ub:conv.fpint.float.not.represented) is resolved *per site* and honors source
// line ranges, consistently with every other implicit UB check.  Two
// conversions in the SAME namespace on different lines resolve differently.
//
// Config (p3100-float-cast-location.json), first match wins:
//   1. line 23 of this file -> ignore   (defined 0, no handler)
//   2. default              -> observe  (handler runs, then defined 0)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-float-cast-location.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation (const cs::contract_violation&) { ++calls; }

namespace same_ns {
  // Same namespace, different lines: only the line range distinguishes them.
  int cvt_ignored (double d) { return (int) d; }    // line 23: ignore
  int cvt_observed (double d) { return (int) d; }    // line 24: observe
}

int main () {
  const double big = 1e300;   // truncates to a value not representable in int

  // Line-matched -> ignore: defined 0, no handler call.
  if (same_ns::cvt_ignored (big) != 0) __builtin_abort ();
  if (calls != 0) __builtin_abort ();

  // Out of range -> observe: handler runs, then defined 0.
  if (same_ns::cvt_observed (big) != 0) __builtin_abort ();
  if (calls != 1) __builtin_abort ();
}
