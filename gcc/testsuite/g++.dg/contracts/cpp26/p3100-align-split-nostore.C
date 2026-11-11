// P3100: configuring only the .nostore leaf of ub:basic.align.object.alignment
// (a creation that performs no store, checked only at a placement new)
// leaves a misaligned access unguarded.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int load (int *p) { return *p; }
// { dg-final { scan-tree-dump-not "__builtin_trap" "optimized" } }
