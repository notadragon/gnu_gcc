// P3100: a pointer-to-data-member access o->*pm whose member is read or
// written is followed by a load or store, so the null check of its object
// pointer is the .access leaf of ub:expr.unary.dereference.nullptr, as for
// o->m; where only the member's address is formed, or a reference is bound
// to it, it is the .indirection leaf.  Configuring only .indirection checks
// `a` in each address shape here -- the address of the member, of an array
// member's element and of a class member's member, a reference bound to the
// member, the member as a discarded-value expression (no lvalue-to-rvalue
// conversion), the `.*` form &((*a).*pm), and the address of a member of a
// class member at a nonzero offset, &(a->s.*pm), whose check is on `a` too
// (GCC-793) -- and nothing in the accessing shapes.
// Mirror: llvm_llvm-project clang/test/Contracts/p3100-null-split.cpp
// (shape_mptr) and p3100-null-split-mptr.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S { int m; int n; int arr[4]; };
struct T { int t; S s; };
int read (S *o1, int S::*pm) { return o1->*pm; }
void store (S *o2, int S::*pm) { o2->*pm = 1; }
void compound (S *o3, int S::*pm) { o3->*pm += 1; }
void incr (S *o4, int S::*pm) { ++(o4->*pm); }
int arr (S *o5, int (S::*pa)[4], int i) { return (o5->*pa)[i]; }
int chain (T *o6, S T::*ps) { return (o6->*ps).m; }
int *addr (S *a1, int S::*pm) { return &(a1->*pm); }
int *arr_addr (S *a2, int (S::*pa)[4], int i) { return &(a2->*pa)[i]; }
int *chain_addr (T *a3, S T::*ps) { return &(a3->*ps).m; }
int &ref (S *a4, int S::*pm) { return a4->*pm; }
void discard (S *a5, int S::*pm) { a5->*pm; }
int dread (S *oa, int S::*pm) { return (*oa).*pm; }
int member (T *ob, int S::*pm) { return ob->s.*pm; }
int *daddr (S *aa, int S::*pm) { return &((*aa).*pm); }
int *member_addr (T *ab, int S::*pm) { return &(ab->s.*pm); }
// { dg-final { scan-tree-dump-not {if \(o[1-6ab][._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(a1[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(a2[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(a3[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(a4[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(a5[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(aa[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(ab[._0-9A-Z()]* == 0B\)} "optimized" } }
