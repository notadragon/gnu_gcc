// D4405R0, "Virtual Functions", with -fcontract-checks-outlined: the
// override shadows' regions stay in the overrider's body, around the call
// of its outlined postcondition function -- whose own shadow would not
// have the overridden function's preconditions -- and the overridden
// function's postconditions are proven after it.  (D::f's postcondition
// is also analyzed in its outlined function, where nothing is known of the
// result, hence diagnose and the warning there.)
//
// GCC-only: -fcontract-checks-outlined has no Clang counterpart
// (notadragon_wg21 DECISIONS.md B6).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-checks-outlined -fcontract-analysis-semantic=diagnose -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow" }

struct B {
  virtual int f (int x) pre (x > 0) post (r: r > 0);	// { dg-message "the predicate of the contract assertion" }
};

struct D : B {
  int f (const int x) override			// { dg-bogus "cannot be proven" }
    pre (x >= 1) post (r: r >= x)		// { dg-warning "contract assertion cannot be proven" }
  { return x; }
};

struct E : B {
  int f (int x) override			// { dg-warning "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
  { return x - 1; }
};

void use () { D d; E e; }

// D::f's precondition and postcondition (line 21) and B::f's postcondition
// (line 16) in D::f's override shadow; B::f's in E::f's.
// { dg-final { scan-tree-dump-times {\(line 21, override pair [0-9]+\): conditional} 2 "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 16, override pair [0-9]+\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 16, override pair [0-9]+\): unproven} "contract_shadow" } }
