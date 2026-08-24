// P3100: the signed-overflow check is per-operation, and lowered in pass_ubsan.
// Under "assume" the addition is left as a raw signed PLUS (the optimizer keeps
// the no-overflow assumption, so loops built on it stay optimized).  Under
// "ignore" the addition becomes a .ADD_OVERFLOW internal function whose real
// part is the defined 2's-complement wrapped result; the internal function both
// yields that defined value AND stops the optimizer assuming no overflow (this
// is what de-optimizes such loops -- see the D3100 overflow celink).  Only the
// ign_ns addition is instrumented; the default (assume) one stays a raw PLUS.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O2 -fdump-tree-ubsan -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-overflow-codegen.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int g_add (int a, int b) { return a + b; }          // assume (builtin default)

namespace ign_ns {
  int add (int a, int b) { return a + b; }          // ignore -> instrumented
}

// Exactly one addition is instrumented (ign_ns::add); g_add stays a raw PLUS.
// { dg-final { scan-tree-dump-times "ADD_OVERFLOW" 1 "ubsan" } }
