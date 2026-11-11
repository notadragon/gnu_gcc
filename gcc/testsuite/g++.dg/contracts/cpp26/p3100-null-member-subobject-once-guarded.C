// P3100: one null dereference through `o->*pm` is checked once -- on `o`,
// not again on the member's address `o + pm` or on a reference bound to it
// -- also when the null member pointer check
// (ub:expr.mptr.oper.member.func.null.data) guards the member's address,
// and for a member of a class member at a nonzero offset, `o->s.*pm`, whose
// check is on `o` too.  (Regression test for GCC-793; the run-time counts
// are in p3100-null-member-subobject-once.C.)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:quick_enforce,ub:expr.mptr.oper.member.func.null.data:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct M { int m; int n; };
struct T { int t; M s; };
int rd (M *o1, int M::*pm) { return o1->*pm; }
int &ref (M *o2, int M::*pm) { return o2->*pm; }
int rd_s (T *o3, int M::*pm) { return o3->s.*pm; }
// { dg-final { scan-tree-dump-times "== 0B" 3 "optimized" } }
// { dg-final { scan-tree-dump {if \(o1[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(o2[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(o3[._0-9A-Z()]* == 0B\)} "optimized" } }
