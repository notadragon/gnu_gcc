// P3100: a call through a null pointer to member function (ub:expr.mptr.oper.member.func.null.function)
// configured to "ignore": no check is emitted (the operation itself is not run here).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S { int f () { return 1; } virtual int g () { return 2; } };
int (S::*volatile pmv) () = nullptr;
__attribute__((noinline)) int call (S &s, int (S::*pm) ()) { return (s.*pm) (); }
// { dg-final { scan-tree-dump-not "contract_violation" "optimized" } }
// { dg-final { scan-tree-dump-not "__builtin_trap" "optimized" } }
