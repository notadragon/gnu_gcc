// D4405R0, "Virtual Functions": every function a virtual function
// overrides, directly or indirectly, gets an override shadow of its own,
// which takes that function's preconditions as given and proves the
// overrider's at entry; `this' is the base class subobject.  Each
// namespace's use () constructs the classes, which emits their vtables and
// with them the overriders (DECISIONS.md E150).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }
// { dg-additional-options "-fdump-ipa-contract_shadow_clone" }

// D's call through B evaluates B::f's and D::f's preconditions, never
// C::f's: D::f is analyzed against B::f and against C::f separately, and
// C::f's shadow against B::f does not cover D::f.
namespace multilevel {
  struct B { virtual void f (int x) pre (x > 0); };
  struct C : B {
    void f (int x) override	// { dg-error "cannot be proven for a call through 'virtual void multilevel::B::f\\(int\\)'" }
      pre (x > 5) {}		// { dg-message "the predicate of the contract assertion" }
  };
  struct D : C {
    void f (int x) override	// { dg-error "cannot be proven for a call through 'virtual void multilevel::B::f\\(int\\)'" }
      pre (x > 2) {}		// { dg-message "the predicate of the contract assertion" }
  };
  void use () { C c; D d; }
}

// Each base of a class with several: B is not D's primary base, so `this'
// is adjusted, and B::m is the same object in the premise and in D::f's
// precondition.
namespace multiple {
  struct A { virtual void f (int x) pre (x > 0); };
  struct B { int m; virtual void f (int x) pre (x > 10 && m > 0); };
  struct D : A, B {
    void f (int x) override	// { dg-error "cannot be proven for a call through 'virtual void multiple::A::f\\(int\\)'" }
      pre (x > 5 && m > 0) {}	// { dg-message "the predicate of the contract assertion" }
  };
  void use () { D d; }
}

// A virtual base, reached through two paths: one override shadow, with
// `this' adjusted through the vbase offset.
namespace vbase {
  struct V { int m; virtual int f () pre (m > 0); };
  struct L : virtual V {};
  struct R : virtual V {};
  struct D : L, R {
    int f () override pre (m > 0) { return m; }	// { dg-bogus "cannot be proven" }
  };
  struct E : L, R {
    int f () override		// { dg-error "cannot be proven for a call through 'virtual int vbase::V::f\\(\\)'" }
      pre (m > 1) { return m; }	// { dg-message "the predicate of the contract assertion" }
  };
  void use () { D d; E e; }
}

// A pure virtual function's preconditions are still the premises.
namespace pure {
  struct B { virtual int f (int x) pre (x > 0) = 0; };
  struct D : B {
    int f (int x) override pre (x >= 1) { return x; }	// { dg-bogus "cannot be proven" }
  };
  struct E : B {
    int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int pure::B::f\\(int\\)'" }
      pre (x > 1) { return x; }	// { dg-message "the predicate of the contract assertion" }
  };
  void use () { D d; E e; }
}

// An overridden function without contract assertions still gives an
// override shadow, with no premises: a call through B may pass any
// argument, so D::f's precondition must be a tautology (DECISIONS.md
// E156).
namespace nocontracts {
  struct B { virtual int f (int x); };
  struct D : B {
    int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int nocontracts::B::f\\(int\\)'" }
      pre (x > 100) { return x; }	// { dg-message "the predicate of the contract assertion" }
  };
  struct E : B {
    int f (int x) override pre (x > 100 || x <= 100) { return x; }	// { dg-bogus "cannot be proven" }
  };
  void use () { D d; E e; }
}

// An overrider not defined in this translation unit has no body to analyze.
namespace declonly {
  struct B { virtual int f (int x) pre (x > 0); };
  struct D : B { int f (int x) override pre (x > 100); };	// { dg-bogus "cannot be proven" }
}

// multilevel: C (one), D (two); multiple: D (two); vbase: D, E (one each);
// pure: D, E (one each); nocontracts: D (one; E::f's precondition
// folds to true).
// { dg-final { scan-ipa-dump-times "Override shadow" 10 "contract_shadow_clone" } }
