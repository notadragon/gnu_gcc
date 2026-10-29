// Under P3595 caller-side checking (caller observe, callee ignore), a call
// of a constructor whose object is a prvalue -- `T ()', `T (1)', `T {}', a
// value-initialized member, `new T ()', a returned `T ()' -- is checked
// like any other call (GCC-921: the caller-side wrapper is not a
// constructor, so the call initializes its temporary itself, which was an
// internal compiler error).  The temporary is still elided: the object
// constructed is the one initialized.  A base subobject initialized from a
// prvalue of its own type is copied instead (the wrapper calls the complete
// object constructor).
//
// No Clang mirror: Clang has no caller-side checking.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/open-bug-caller-side.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
static int g = 0;

void handle_contract_violation (const std::contracts::contract_violation&)
{
  ++violations;
}

struct T {
  T *self;
  int v;
  T () pre (g > 0) : self (this), v (1) {}
  T (int x) pre (g > x) : self (this), v (x) {}
  T (const T &) = delete;
};

struct V : virtual T {
  V () pre (g > 0) : T (2) {}
  V (const V &o) : T (o.v) {}
};

struct H {
  T t;
  H () : t () {}
};

struct C {
  C *self;
  C () pre (g > 0) : self (this) {}
  C (const C &) : self (this) {}
};

struct B : C {
  B () : C (C ()) {}
};

T ret () { return T (); }

int main ()
{
  int n = 0;
  if (T ().v != 1 || violations != ++n)
    __builtin_abort ();
  if (T (3).v != 3 || violations != ++n)
    __builtin_abort ();
  if (T {}.v != 1 || violations != ++n)
    __builtin_abort ();
  T a = T ();
  if (a.self != &a || violations != ++n)
    __builtin_abort ();
  H h;
  if (h.t.self != &h.t || violations != ++n)
    __builtin_abort ();
  T *p = new T ();
  if (p->self != p || violations != ++n)
    __builtin_abort ();
  delete p;
  T r = ret ();
  if (r.self != &r || violations != ++n)
    __builtin_abort ();
  B b;
  if (b.self != static_cast<C *> (&b) || violations != ++n)
    __builtin_abort ();
  V w = V ();
  if (w.self != static_cast<T *> (&w) || violations != (n += 2))
    __builtin_abort ();
}
