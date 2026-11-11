// P3100: with the floating-point division check
// (ub:expr.mul.div.by.zero.fp.div) configured, a divisor that is a nonzero
// constant is not guarded.
// Mirror: llvm_llvm-project clang/test/Contracts/p3100-fdiv-int-unaffected.cpp (fhalf).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-gimple" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.fp.div:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

double fhalf (double a) { return a / 2.0; }
double fneg (double a) { return a / -0.5; }
// { dg-final { scan-tree-dump-not "__builtin_trap \\(\\);" "gimple" } }
