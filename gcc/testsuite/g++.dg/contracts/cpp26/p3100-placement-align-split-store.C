// P3100: configuring only the .store leaf of ub:basic.align.object.alignment
// leaves a placement new that stores nothing unchecked: that is the .nostore
// leaf.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <new>
int *make (void *p) { return new (p) int; }
// { dg-final { scan-tree-dump-not "__builtin_trap" "optimized" } }
