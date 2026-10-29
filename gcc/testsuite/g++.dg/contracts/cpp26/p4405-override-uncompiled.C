// D4405R0, "Virtual Functions", DECISIONS.md E150: the override obligations
// of a virtual function this translation unit never compiles -- an inline
// overrider whose class's vtable is emitted elsewhere, here because the key
// function is defined in another translation unit -- are not diagnosed;
// that translation unit compiles the overrider and analyzes it.  Ordinary
// analysis evaluations keep E130's rule.  Kept and used, the same class is
// diagnosed.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }

struct B { virtual int f (int x) pre (x > 0); };

struct Elsewhere : B {
  virtual void key ();		// defined in another translation unit
  int f (int x) override	// { dg-bogus "cannot be proven" }
    pre (x > 10)
  { return x; }
};

struct Here : B {
  virtual void key ();
  int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
    pre (x > 10)		// { dg-message "the predicate of the contract assertion" }
  { return x; }
};

void Here::key () {}

// Decided at once (folding): diagnosed only if compiled as well.
struct ElsewhereFalse : B {
  virtual void key ();
  int f (int x) override	// { dg-bogus "always false" }
    pre (false)
  { return x; }
};

struct HereFalse : B {
  virtual void key ();
  int f (int x) override	// { dg-error "contract predicate is always false for a call through 'virtual int B::f\\(int\\)'" }
    pre (false)			// { dg-message "the predicate of the contract assertion" }
  { return x; }
};

void HereFalse::key () {}
