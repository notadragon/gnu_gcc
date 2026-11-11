// D4405's proof optimization level (DECISIONS.md E162-E168, E205):
// -fcontracts-prove-optimize=0 -fcontracts-prove-shadow at -O2: the
// explicit flag turns the shadow on (E164), at level 0, optimized as at
// -O0 (E208 (a)), while override and assume stay off.
//
// Each strategy is on from a proof level (fold 0, shadow and assume 1,
// override 2): without override shadows, D::f's case is not proven; D::g
// is proven in its proof shadow and D::h by folding.  k's evaluation needs the shadow's optimization,
// which level 0 does not give it, so it is unproven.  Without
// -fcontracts-prove-assume no assumption region is placed (no
// CONTRACT_PROVEN in the gimple dump).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow" }
// { dg-additional-options "-fdump-ipa-contract_shadow_clone -fdump-tree-gimple" }

struct B {
  virtual int f (int x) pre (x > 0) post (r: r > 0);	// { dg-message "the predicate of the contract assertion" }
  virtual int g (int x) pre (x > 0) post (r: r > 0);
  virtual int h (int x) pre (x > 0);
};

struct D : B {
  // Proven only in an override shadow, from B::f's precondition: off here.
  int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
  { return x; }
  // Proven in D::g's proof shadow, at level 0 too: the last look at the
  // condition guarding the evaluation finds it.
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

// { dg-final { scan-ipa-dump-not "Override shadow" "contract_shadow_clone" } }
// { dg-final { scan-tree-dump-not "CONTRACT_PROVEN" "gimple" } }

// A -O0 build at proof level 0 -- given, or by follow -- gives the same
// diagnostics.
// { dg-final { contracts-same-diagnostics "" "-O0" "-O0 -fcontracts-prove-optimize=follow" } }
