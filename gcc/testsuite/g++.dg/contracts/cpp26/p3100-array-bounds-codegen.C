// P3100: an out-of-bounds subscript is guarded per site.  Under "assume" (no
// config) the subscript is a plain g[i].  Under "ignore" the index is replaced
// by `(unsigned)i >= bound ? 0 : i`, redirecting an out-of-range access to the
// valid index 0 -- see cp_build_implicit_bounds_check.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O2 -fdump-tree-original -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-array-bounds-codegen.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int g[8];
int a_rd (int i) { return g[i]; }                    // assume (no config)

namespace ign_ns {
  int rd (int i) { return g[i]; }                    // ignore -> guarded index
}

// Exactly one guarded subscript (ign_ns::rd); a_rd stays a plain g[i].
// { dg-final { scan-tree-dump-times "\\? 0 :" 1 "original" } }
