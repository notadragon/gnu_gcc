// P3100: with the known-bound pointer-arithmetic check configured, a constant
// in-range offset at run time emits no check.  (p3100-arith-bounds-constinit.C
// has the constant initializers.)
// Mirror: llvm_llvm-project clang/test/Contracts/p3100-arith-bounds-constinit.cpp (from).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-gimple" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int g[4];
int *from () { int *volatile q = g; (void) q; return g + 3; }
int *to_end () { return g + 4; }
// { dg-final { scan-tree-dump-not "__builtin_trap \\(\\);" "gimple" } }
