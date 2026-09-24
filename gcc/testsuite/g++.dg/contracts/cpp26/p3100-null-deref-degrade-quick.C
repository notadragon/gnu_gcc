// P3100: without -fcontracts-p4298 there is no nothrow handler variant, so a
// configured (but unsupported) throwing "observe" for the null-dereference
// check clamps -- via the fallback order -- to "quick_enforce" (a trap), rather
// than being diagnosed.  Verified in the dump: the guarded load lowers to a
// conditional __builtin_trap.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O0 -fdump-tree-optimized -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-null-deref-degrade-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int load_it (int *p) { return *p; }

// { dg-final { scan-tree-dump-times "__builtin_trap" 1 "optimized" } }
