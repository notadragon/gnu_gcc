// P3100: under quick_enforce, each unaccessed indirection through a pointer
// that may be null gets one guard (GCC-799); `this', the address of an
// object and a reference are not checked.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized -Wno-unused-value" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S
{
  static int k;
  static int sf ();
  int m;
  int g ();
};

int
S::g ()
{
  (void) *this;
  S *q = &*this;
  return this->sf () + (*this).sf () + this->k + (q == this);
}

int *addr (int *p) { return &*p; }
void disc (int *p) { *p; }
int sfc (S *sp) { return sp->sf (); }
int sk (S *sp) { return sp->k; }

int x;
int *addr_obj () { return &*&x; }
int sfc_ref (S &r) { return r.sf (); }
void disc_ref (int &r) { r; }

// { dg-final { scan-tree-dump-times "__builtin_trap" 4 "optimized" } }
