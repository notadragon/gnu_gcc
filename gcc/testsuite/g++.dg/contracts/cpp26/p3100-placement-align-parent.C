// P3100: the placement-new alignment check, the .nostore leaf, is selected
// by its group ub:basic.align.object.alignment too.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-original" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <new>
int *make (void *p) { return new (p) int; }
// { dg-final { scan-tree-dump-times "__builtin_trap" 1 "original" } }
