// D4405R0, "Virtual Functions", with neither override shadows nor proof
// shadows (-fno-contracts-prove-override -fno-contracts-prove-shadow,
// DECISIONS.md E157): every override obligation is decided at once, so
// only folding settles one, whether the overrider's precondition or the
// overridden function's postcondition.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fno-contracts-prove-shadow -fcontract-analysis-semantic=diagnose -O2" }

struct B {
  virtual int f (int x) pre (x > 0) post (r: r > 0);	// { dg-message "the predicate of the contract assertion" }
  virtual int g (int x) post (r: true);
};

struct D : B {
  int f (int x) override	// { dg-warning "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
    pre (x > 0)			// { dg-message "the predicate of the contract assertion" }
  { return 1; }
  int g (int x) override { return x; }	// { dg-bogus "cannot be proven" }
};

void use () { D d; }
