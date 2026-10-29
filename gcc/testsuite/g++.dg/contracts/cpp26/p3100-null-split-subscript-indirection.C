// P3100: a member access whose object expression is a subscript, p[i].n, and an
// element of a two-dimensional member array, p->a2[i][j], report the .access
// leaf of ub:expr.unary.dereference.nullptr where the member or element is read
// and the .indirection leaf where only its address is formed.  Configuring only
// .indirection guards the two address shapes: `&p[i].n` (on `p + i`, see below)
// and `&p->a2[i][j]` (on `p4`), and neither reading shape.
// GCC checks the element's address `p + i` of `p[i].n` and `&p[i].n`,
// so a null `p` with i != 0 is missed; Clang checks `p` (divergence
// GCC-794, CLANG-745; DECISIONS.md E83 is open).
// Mirror: llvm_llvm-project clang/test/Contracts/p3100-null-split.cpp
// (shape_sub_rd, shape_sub_addr, shape_arr2_rd, shape_arr2_addr).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S { int m; int n; };
struct U { int z; int a2[2][2]; };
int sub_rd (S *p1, int i) { return p1[i].n; }
int *sub_addr (S *p2, int i) { return &p2[i].n; }
int arr2_rd (U *p3, int i, int j) { return p3->a2[i][j]; }
int *arr2_addr (U *p4, int i, int j) { return &p4->a2[i][j]; }
// { dg-final { scan-tree-dump-times "__builtin_trap" 2 "optimized" } }
// { dg-final { scan-tree-dump {if \(p4[._0-9A-Z()]* == 0B\)} "optimized" } }
// { dg-final { scan-tree-dump-not {if \(p3[._0-9A-Z()]* == 0B\)} "optimized" } }
