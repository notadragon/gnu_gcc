// P3100: the implicit null-dereference semantic is resolved *per site* from the
// P3595 configuration, distinguishing source location (file + line range) and
// namespace, with first-match-wins precedence.  We verify this by counting the
// __builtin_trap reactions (quick_enforce) the middle end emits: a site traps
// iff it resolves to quick_enforce.
//
// Config (see p3100-null-deref-resolution.json), first match wins:
//   1. namespace "assume_ns"                 -> assume         (no trap)
//   2. location lines 30-32 of this file     -> quick_enforce  (trap)
//   3. default (any other implicit deref)    -> assume         (no trap)
//
// Compiled at -O0 so the optimizer cannot introduce or remove traps on its own.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O0 -fdump-tree-optimized -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-null-deref-resolution.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

// Out of the quick line-range and not in assume_ns: default -> assume, no trap.
int g_out (int *p) { return *p; }

namespace assume_ns {
  // In assume_ns, out of the range: namespace entry -> assume, no trap.
  int a_out (int *p) { return *p; }
}

// The next two dereferences sit on lines 30 and 32, inside the quick line-range.
namespace assume_ns {
  int a_in (int *p) { return *p; }   // line 30: matches BOTH ns(1) and line(2);
}                                     //          first match (namespace) wins -> assume, no trap
int g_in (int *p) { return *p; }     // line 32: matches only line(2) -> quick_enforce, TRAP

// Exactly one site (g_in) resolves to quick_enforce.
// { dg-final { scan-tree-dump-times "__builtin_trap" 1 "optimized" } }
