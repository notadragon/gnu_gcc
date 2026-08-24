// P3100: the implicit flow-off-end semantic (ub:stmt.return.flow.off) is
// resolved *per site* and honors source line ranges, consistently with every
// other implicit UB check.  For flow-off the "site" is the function's
// declaration line (DECL_SOURCE_LOCATION), so two value-returning functions in
// the SAME namespace declared on different lines resolve differently.
//
// Config (p3100-flow-off-location.json), first match wins:
//   1. line 24 of this file -> ignore   (defined zeroed return, no handler)
//   2. default              -> observe  (handler runs, then zeroed return)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-flow-off-location.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation (const cs::contract_violation&) { ++calls; }

namespace same_ns {
  // Same namespace, different declaration lines: only the line range differs.
  int f_ignored (int x) { if (x) return x; }    // line 24: ignore, falls off
  int f_observed (int x) { if (x) return x; }    // line 25: observe, falls off
}

int main () {
  // Line-matched -> ignore: zeroed return, no handler call.
  if (same_ns::f_ignored (0) != 0) __builtin_abort ();
  if (calls != 0) __builtin_abort ();

  // Out of range -> observe: handler runs, then zeroed return.
  if (same_ns::f_observed (0) != 0) __builtin_abort ();
  if (calls != 1) __builtin_abort ();
}
