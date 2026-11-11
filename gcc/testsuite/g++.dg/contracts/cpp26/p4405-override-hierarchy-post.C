// D4405R0, "Virtual Functions", the return side through several levels
// and several bases: each overridden function's postconditions are proven
// in that function's own override shadow, and `this' in them is its base
// class subobject.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }
// { dg-additional-options "-fdump-ipa-contract_shadow_clone" }

// D's call through B evaluates B::f's postconditions and through C, C::f's:
// E::f satisfies B::f's but not C::f's.
namespace multilevel {
  struct B { virtual int f (int x) post (r: r > 0); };
  struct C : B {
    int f (int x) override post (r: r > 1) { return 5; }	// { dg-bogus "cannot be proven" }
  };
  struct D : C {
    int f (int x) override { return 2; }	// { dg-bogus "cannot be proven" }
  };
  struct E : C {
    int f (int x) override	// { dg-error "for a call through 'virtual int multilevel::C::f\\(int\\)'" }
    { return 1; }
    // { dg-message "the predicate of the contract assertion" "" { target *-*-* } 14 }
  };
  void use () { C c; D d; E e; }
}

// B is not D's primary base: B::f's `m' is read through D's `this'
// adjusted to the B subobject, the same object D::f's body writes.
namespace multiple {
  struct A { virtual int f (int x) post (r: r > 0); };	// { dg-message "the predicate of the contract assertion" }
  struct B { int m; virtual int f (int x) post (r: r == m); };
  struct D : A, B {
    int f (int x) override { m = 7; return 7; }	// { dg-bogus "cannot be proven" }
  };
  struct E : A, B {
    int f (int x) override	// { dg-error "for a call through 'virtual int multiple::A::f\\(int\\)'" }
    { m = 0; return 0; }
  };
  void use () { D d; E e; }
}

// multilevel: C (one), D and E (two each); multiple: D and E (two each).
// { dg-final { scan-ipa-dump-times "Override shadow" 9 "contract_shadow_clone" } }
