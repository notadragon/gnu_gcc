// P3100: under quick_enforce, each of GCC-804's unaccessed indirections
// through a pointer that may be null gets one guard under the .indirection
// leaf; a discarded conversion of `*p' to a virtual base reads through p,
// so it is the .access leaf's, and the address of an object, a reference
// and `this' are not checked.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized -Wno-unused-value" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S
{
  int m;
  enum { E = 5 };
  static int k;
  int g ();
};
struct A { int a; };
struct D : A, S {};
struct V : virtual S {};
using I = int;

int *addr_cond (int *p, int *q, bool b) { return &(b ? *p : *q); }
int *addr_comma (int *p) { return &(0, *p); }
S *addr_cast (S *sp) { return &static_cast<S &> (*sp); }
void disc_cast (S *sp) { static_cast<S &> (*sp); }
void disc_base_off (D *dp) { static_cast<S &> (*dp); }
int *baddr (int *p) { return __builtin_addressof (*p); }
int se (S *sp) { return sp->E; }
void pdtor (int *p) { p->~I (); }
void sdtor (S *sp) { sp->~S (); }

// Not checked.
void disc_vbase (V *vp) { static_cast<S &> (*vp); }
int x, y;
int *addr_obj_cond (bool b) { return &(b ? x : y); }
void disc_ref_cast (int &r) { static_cast<int &> (r); }
int S::g () { static_cast<S &> (*this); return (*this).E + this->k; }

// { dg-final { scan-tree-dump-times "__builtin_trap" 9 "optimized" } }
