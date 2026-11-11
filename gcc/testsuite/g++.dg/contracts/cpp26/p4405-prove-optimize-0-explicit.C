// D4405's proof optimization level (DECISIONS.md E162-E168, E205):
// -fcontracts-prove-optimize=0 with every strategy given explicitly at
// -O2: the explicit flags win over the level (E164), and the shadows run
// at level 0, optimized as at -O0 (E208 (a)).
//
// Each strategy is on from a proof level (fold 0, shadow and assume 1,
// override 2).  D::h is proven by folding.  B::f's and B::g's
// postconditions are still proven in D::f's and D::g's override shadows:
// the last look at a condition guarding an evaluation, after the shadow's
// optimization, finds them, whatever the level.  k's evaluation needs the
// shadow's optimization, which level 0 does not give it, so it is
// unproven.  -fcontracts-prove-assume places its assumption regions
// (CONTRACT_PROVEN in the gimple dump).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-assume" }
// { dg-additional-options "-fdump-ipa-contract_shadow_clone -fdump-tree-gimple" }

struct B {
  virtual int f (int x) pre (x > 0) post (r: r > 0);
  virtual int g (int x) pre (x > 0) post (r: r > 0);
  virtual int h (int x) pre (x > 0);
};

struct D : B {
  // B::f's postcondition is proven in an override shadow, from B::f's
  // precondition.
  int f (int x) override	// { dg-bogus "(cannot be proven|always false)" }
  { return x; }
  // B::g's postcondition is proven in an override shadow.
  int g (int x) override	// { dg-bogus "(cannot be proven|always false)" }
  { return x > 0 ? x : 1; }
  // A tautology, proven by folding.
  int h (int x) override pre (true) { return x; }	// { dg-bogus "(cannot be proven|always false)" }
};

void use () { D d; }

// Proven in k's proof shadow at level 1 and up, not at level 0.
int k (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-error "contract assertion cannot be proven" }
				// { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-1 }
  return i;
}

// { dg-final { scan-ipa-dump "Override shadow" "contract_shadow_clone" } }
// { dg-final { scan-tree-dump "CONTRACT_PROVEN" "gimple" } }

// A -O0 build at proof level 0 -- given, or by follow -- gives the same
// diagnostics.
// { dg-final { contracts-same-diagnostics "" "-O0" "-O0 -fcontracts-prove-optimize=follow" } }
