// P3100: the null-dereference check reports the .access leaf of
// ub:expr.unary.dereference.nullptr where the indirection is followed by a
// load or store, and the .indirection leaf where it is not, for the member
// shapes beyond p3100-null-split-access.C's.  Configuring only .access guards
// the three accessing shapes here: a store p->n = 1, a read of p->arr[i] and a
// read p->s.m.
// (Pointers to data members: p3100-null-split-mptr-*.C.)
// Mirror: llvm_llvm-project clang/test/Contracts/p3100-null-split.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S { int m; int n; int arr[4]; void f (); };
struct T { S s; };
void shape_store (S *p) { p->n = 1; }
int shape_arr_access (S *p, int i) { return p->arr[i]; }
int *shape_arr_addr (S *p, int i) { return &p->arr[i]; }
int shape_chain (T *p) { return p->s.m; }
int *shape_chain_addr (T *p) { return &p->s.m; }
// { dg-final { scan-tree-dump-times "__builtin_trap" 3 "optimized" } }
