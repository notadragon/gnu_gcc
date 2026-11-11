// P3100: the null check of the object pointer of a pointer-to-data-member
// access whose member is read is the .access leaf of
// ub:expr.unary.dereference.nullptr (see p3100-null-split-mptr-access.C)
// also when the null member pointer check
// (ub:expr.mptr.oper.member.func.null.data) guards the member's address, in
// a function template's instantiation and in a lambda; the address shape
// keeps the .indirection leaf.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce,ub:expr.mptr.oper.member.func.null.data:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S { int m; int n; };
int read (S *o1, int S::*pm) { return o1->*pm; }
template <typename U> int tread (U *o2, int U::*pm) { return o2->*pm; }
template int tread<S> (S *, int S::*);
int lread (S *p, int S::*pm)
{
  return [] (S *o3, int S::*q) { return o3->*q; } (p, pm);
}
int *addr (S *a1, int S::*pm) { return &(a1->*pm); }
// { dg-final { scan-tree-dump {if \(o1[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(o2[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump {if \(o3[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump-not {if \(a1[._0-9A-Z()]* == 0B\)} "optimized" } }
