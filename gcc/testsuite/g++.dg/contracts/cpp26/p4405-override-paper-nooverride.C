// D4405R0, "Virtual Functions", without override shadows
// (-fno-contracts-prove-override, DECISIONS.md E157): each override
// obligation is one of the overrider itself, with no premise from the
// overridden function.  D::f's precondition `x >= 0`, proven in its
// override shadow from B::f's `x > 0` (p4405-override-paper.C), must now
// be a tautology, and is diagnosed; B::f's postcondition `r > 0` is an
// obligation in D::f's ordinary shadow, proven there from D::f's own
// precondition, enforced at entry, and in E::f's from E::f's.  D::f's own
// postcondition needs nothing beyond its analysis evaluation.  No override
// shadow is made.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fcontract-analysis-semantic=prove -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-ipa-contract_shadow_clone" }

struct B {
  virtual int f(int x)
    pre(x > 0)
    post(r: r > 0);
};

struct D : B {
  int f(const int x) override	// { dg-error "contract assertion cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
    pre(x >= 0)			// { dg-message "the predicate of the contract assertion" }
    post(r: r > x)
  {
    return x + 1;
  }
};

struct E : B {
  int f(int x) override		// { dg-error "contract assertion cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
    pre(x > 10)			// { dg-message "the predicate of the contract assertion" }
  {
    return x;
  }
};

B *make (bool d) { return d ? static_cast<B *> (new D) : new E; }

// { dg-final { scan-ipa-dump-not "Override shadow" "contract_shadow_clone" } }
// { dg-final { scan-tree-dump-times {\(line 18, override pair [0-9]+ in the ordinary shadow\): sound} 2 "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 24\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump-not {\(line 2[23], override pair} "contract_shadow" } }
