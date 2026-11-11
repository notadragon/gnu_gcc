// P3100: a pointer-to-data-member access o->*pm whose member is read or
// written is followed by a load or store, so the null check of its object
// pointer `o` is the .access leaf of ub:expr.unary.dereference.nullptr, as
// for o->m; where only the member's address is formed, a reference is bound
// to it, or it is discarded, it is the .indirection leaf.  Configuring only
// .access checks `o` in each accessing shape here -- a read, a store, a
// compound assignment, an increment, a read of an array member's element, a
// read of a class member's member, a member of a base class at a nonzero
// offset and a member of a virtual base, the `.*` form (*o).*pm, and a
// member of a class member at a nonzero offset, o->s.*pm, whose check is on
// `o` too (GCC-793) -- and `a` in none of the other shapes.  (The check is on `o` itself: the loaded address o + pm is not
// null for a null `o` and a member at a nonzero offset.)
// Mirror: llvm_llvm-project clang/test/Contracts/p3100-null-split.cpp
// (shape_mptr), p3100-null-split-mptr.cpp and
// Runnable/p3100-null-split-mptr-offset.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S { int m; int n; int arr[4]; };
struct T { int t; S s; };
struct B0 { int z; };
struct D : B0, S { };
struct V : virtual S { };
int read (S *o1, int S::*pm) { return o1->*pm; }
void store (S *o2, int S::*pm) { o2->*pm = 1; }
void compound (S *o3, int S::*pm) { o3->*pm += 1; }
void incr (S *o4, int S::*pm) { ++(o4->*pm); }
int arr (S *o5, int (S::*pa)[4], int i) { return (o5->*pa)[i]; }
int chain (T *o6, S T::*ps) { return (o6->*ps).m; }
int base (D *o7, int S::*pm) { return o7->*pm; }
int vbase (V *o8, int S::*pm) { return o8->*pm; }
int *addr (S *a1, int S::*pm) { return &(a1->*pm); }
int *arr_addr (S *a2, int (S::*pa)[4], int i) { return &(a2->*pa)[i]; }
int *chain_addr (T *a3, S T::*ps) { return &(a3->*ps).m; }
int &ref (S *a4, int S::*pm) { return a4->*pm; }
void discard (S *a5, int S::*pm) { a5->*pm; }
int dread (S *oa, int S::*pm) { return (*oa).*pm; }
int member (T *ob, int S::*pm) { return ob->s.*pm; }
int *daddr (S *aa, int S::*pm) { return &((*aa).*pm); }
int *member_addr (T *ab, int S::*pm) { return &(ab->s.*pm); }
// { dg-final { scan-tree-dump {if \(o1[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(o2[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(o3[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(o4[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(o5[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(o6[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(o7[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(o8[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(oa[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(ob[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump-not {if \(a[1-5ab][._0-9A-Z()]* == 0B\)} "optimized" } }
