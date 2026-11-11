// P3100: configuring only the floating-point leaf
// ub:expr.mul.div.by.zero.fp.div leaves integer division by zero
// (ub:expr.mul.div.by.zero.int) at the default "assume": no integer guard is
// emitted, while the floating-point division gets its trap.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-original" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.fp.div:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int idiv (int a, int b) { return a / b; }
double fdiv (double a, double b) { return a / b; }
// { dg-final { scan-tree-dump-times "__builtin_trap" 1 "original" } }
