// D4405R0, "Virtual Functions": an overrider's postconditions are proven
// in each override shadow at each return, ahead of their own checks, with
// the overridden function's preconditions as premises, and again in the
// overrider's ordinary shadow, without them -- distinct analysis
// evaluations, each diagnosed on its own.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow" }

// Provable for a call through B, from B::f's precondition, but not for a
// direct call.
namespace through {
  struct B { virtual int f (int x) pre (x > 0); };
  struct D : B {
    int f (const int x) override	// { dg-bogus "for a call through" }
      post (r: r > 0)		// { dg-warning "contract assertion cannot be proven" }
    { return x; }
  };
  void use () { D d; }
}

// An obligation is never proven by its own check: D::f's enforced
// postcondition follows its analysis evaluations, so neither the override
// shadow (B::f has no contract assertions, so it gives no premise:
// DECISIONS.md E156) nor the ordinary shadow proves it.
namespace own {
  struct B { virtual int f (int x); };
  struct D : B {
    int f (const int x) override	// { dg-warning "contract assertion cannot be proven for a call through 'virtual int own::B::f\\(int\\)'" }
      post (r: r > 0)		// { dg-warning "contract assertion cannot be proven" }
    { return x; }		// { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-1 }
  };
  void use () { D d; }
}

// B::f's postcondition is proven after D::f's enforced check of the same
// predicate, which comes first; D::f's postcondition itself is not.
namespace order {
  struct B { virtual int f (int x) post (r: r > 0); };
  struct D : B {
    int f (const int x) override	// { dg-warning "contract assertion cannot be proven for a call through 'virtual int order::B::f\\(int\\)'" }
      post (r: r > 0)		// { dg-warning "contract assertion cannot be proven" }
    { return x; }		// { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-1 }
  };
  void use () { D d; }
}

// through: D::f's postcondition (line 16) proven in the override shadow;
// order: B::f's (line 39) proven in D::f's, relying on D::f's ordinary
// obligation (line 42), which is unproven, so conditionally.
// { dg-final { scan-tree-dump {\(line 16, override pair [0-9]+\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 16\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 39, override pair [0-9]+\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 42, override pair [0-9]+\): unproven} "contract_shadow" } }
