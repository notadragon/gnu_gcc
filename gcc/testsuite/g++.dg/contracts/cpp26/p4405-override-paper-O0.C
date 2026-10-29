// D4405R0, "Virtual Functions", the paper's example of overriding the
// functions of two bases, at -O0: no override shadow and no proof shadow,
// so folding is the only strategy and the override obligations are those
// of -fno-contracts-prove-override (DECISIONS.md E157), each decided at
// once.  None of D::f's preconditions and postconditions is a tautology,
// so each of D::f's override obligations is diagnosed for both B1::f and
// B2::f, as are E::f's for B1::f, besides D::f's own postcondition and
// both calls of B1::f in g.  GCC-947: the other errors in the translation
// unit -- every one of them decided at once, so the middle end never runs
// -- used to hide every override diagnostic.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O0" }

struct B1 {
  virtual int f(int x)
    pre(x >= 0 && x <= 10)	// { dg-message "the predicate of the contract assertion" }
    post(r: r >= 0 && r <= 10);	// { dg-message "the predicate of the contract assertion" }
};

struct B2 {
  virtual int f(int x)
    pre(x >= 20 && x <= 30)
    post(r: r >= 20 && r <= 30); // { dg-message "the predicate of the contract assertion" }
};

struct D : B1, B2 {
  int f(const int x) override	// { dg-error "cannot be proven for a call through 'virtual int B1::f\\(int\\)'" }
  // { dg-error "cannot be proven for a call through 'virtual int B2::f\\(int\\)'" "" { target *-*-* } .-1 }
    pre(x >= 0 && x <= 30)	// { dg-message "the predicate of the contract assertion" }
    post(r: r == x)		// { dg-error "contract assertion cannot be proven" }
    // { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-1 }
  {
    return x;
  }
};

struct E : B1 {
  int f(int x) override		// { dg-error "cannot be proven for a call through 'virtual int B1::f\\(int\\)'" }
    pre(x >= 5 && x <= 10)	// { dg-message "the predicate of the contract assertion" }
  {
    return x;
  }
};

void g(B1& b, int y)
{
  b.f(5);
  b.f(y);			// { dg-error "contract assertion cannot be proven" }
  if (y >= 0 && y <= 10) {
    b.f(y);			// { dg-error "contract assertion cannot be proven" }
  }
}

D d;
E e;
