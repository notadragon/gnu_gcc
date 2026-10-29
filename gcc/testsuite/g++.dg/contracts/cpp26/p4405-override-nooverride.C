// D4405R0, "Virtual Functions", without override shadows
// (-fno-contracts-prove-override, DECISIONS.md E157): the overrider must
// establish the overridden functions' postconditions as well as its own,
// with no premise from their preconditions -- obligations of its own,
// proven in its ordinary shadow -- and its preconditions must be
// tautologies.  The diagnostics still name the overridden function.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fcontract-analysis-semantic=prove -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-ipa-contract_shadow_clone -fdump-tree-optimized" }

struct B {
  virtual int f (int x) pre (x > 0) post (r: r > 0);	// { dg-message "the predicate of the contract assertion" }
  virtual int g (int x) pre (x > 0) post (r: r > 0);
  virtual int h (int x) pre (x > 0);
};

struct D : B {
  // Proven with an override shadow, from B::f's precondition; not
  // without one.
  int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
  { return x; }
  // Proven without B::g's precondition.
  int g (int x) override	// { dg-bogus "cannot be proven" }
  { return x > 0 ? x : 1; }
  // A tautology, settled by folding.
  int h (int x) override pre (true) { return x; }	// { dg-bogus "cannot be proven" }
};

void use () { D d; }

// { dg-final { scan-ipa-dump-not "Override shadow" "contract_shadow_clone" } }
// { dg-final { scan-tree-dump {\(line 12, override pair [0-9]+ in the ordinary shadow\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 13, override pair [0-9]+ in the ordinary shadow\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump-not "__contract_override" "optimized" } }
