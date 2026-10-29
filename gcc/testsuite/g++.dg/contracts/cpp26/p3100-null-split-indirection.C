// P3100: configuring only the .indirection leaf of
// ub:expr.unary.dereference.nullptr guards the shapes that do not access the
// object: a reference binding, a member call, and the address of a member.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S { int m; int n; void f (); };
int shape_load (int *p) { return *p; }
int &shape_ref (int *p) { int &r = *p; return r; }
void shape_call (S *p) { p->f (); }
int shape_mem (S *p) { return p->m; }
int *shape_addr (S *p) { return &p->n; }
// { dg-final { scan-tree-dump-times "__builtin_trap" 3 "optimized" } }
