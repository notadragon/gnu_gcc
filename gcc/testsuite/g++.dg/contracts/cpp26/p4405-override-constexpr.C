// D4405R0, "Virtual Functions": a constexpr virtual function gets its
// override shadows like any other, and constant evaluation of it is
// unaffected: the override regions are never executed there, nor the
// snapshots they assign (D::g's of `x').
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }

struct B {
  constexpr virtual int f (int x) const pre (x > 0) { return x; }
  constexpr virtual int g (const int x) const post (r: r == x) = 0;
};

struct D : B {
  constexpr int f (int x) const override	// { dg-error "cannot be proven for a call through '.*int B::f.int. const'" }
    pre (x > 10)				// { dg-message "the predicate of the contract assertion" }
  { return x; }
  constexpr int g (int x) const override	// { dg-bogus "cannot be proven" }
  { int r = x; x = 0; return r; }
};

static_assert (D{}.f (20) == 20);
static_assert (D{}.g (5) == 5);

void use () { D d; }
