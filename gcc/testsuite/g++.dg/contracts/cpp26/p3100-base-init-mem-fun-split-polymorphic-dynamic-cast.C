// P3100: configuring the .polymorphic.dynamic.cast leaf of ub:class.base.init.mem.fun
// checks that operation only (1 site(s) in the shared source).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-gimple" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.polymorphic.dynamic.cast:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-base-init-mem-fun-split.h"
// { dg-final { scan-tree-dump-times "__builtin_trap \\(\\);" 1 "gimple" } }
