// P3100: the access-site alignment check is the .store leaf of
// ub:basic.align.object.alignment: a misaligned access is guarded when that
// leaf is configured.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int load (int *p) { return *p; }
// { dg-final { scan-tree-dump-times "__builtin_trap" 1 "optimized" } }
