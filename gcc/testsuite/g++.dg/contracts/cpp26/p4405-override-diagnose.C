// D4405R0, "Virtual Functions": an override obligation analyzed with
// diagnose is a -Wcontract-unproven warning, at the overrider's definition
// and naming the overridden function; "always false" when the overrider's
// precondition folds to false.  (An override shadow relies on the
// overridden function's preconditions, which a caller may not establish,
// so its own answer is never "always false" unless their labels settle
// them.)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -O2" }

struct B { virtual int f (int x) pre (x > 0); };

struct D : B {
  int f (int x) override	// { dg-warning "contract assertion cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
    pre (x > 10)		// { dg-message "the predicate of the contract assertion" }
  { return x; }
};

struct E : B {
  int f (int x) override	// { dg-warning "contract predicate is always false for a call through 'virtual int B::f\\(int\\)'" }
    pre (false)			// { dg-message "the predicate of the contract assertion" }
  { return x; }
};

struct F : B {
  int f (int x) override pre (x > -5) { return x; }	// { dg-bogus "cannot be proven|always false" }
};

void use () { D d; E e; F f; }
