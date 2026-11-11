// P3100: the null-dereference check reports the .access leaf of
// ub:expr.unary.dereference.nullptr at a load or store (*p, p->m), and the
// .indirection leaf where the indirection is not followed by an access.
// Configuring only .access guards the two accessing shapes.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S { int m; int n; void f (); };
int shape_load (int *p) { return *p; }
int &shape_ref (int *p) { int &r = *p; return r; }
void shape_call (S *p) { p->f (); }
int shape_mem (S *p) { return p->m; }
int *shape_addr (S *p) { return &p->n; }
// { dg-final { scan-tree-dump-times "__builtin_trap" 2 "optimized" } }
