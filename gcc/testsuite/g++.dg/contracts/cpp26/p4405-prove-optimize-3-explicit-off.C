// D4405's proof optimization level (DECISIONS.md E162-E168, E205):
// -fcontracts-prove-optimize=3 at -O3 with -fno-contracts-prove-override
// and -fno-contracts-prove-assume: the explicit -fno- forms win over the
// level.
//
// Each strategy is on from a proof level (fold 0, shadow and assume 1,
// override 2): D::f is proven by an override shadow, D::g and k by
// their proof shadows, D::h by folding, and k's evaluation becomes an
// assumption (CONTRACT_PROVEN in the gimple dump) with
// -fcontracts-prove-assume.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O3 -fcontracts-prove-optimize=3 -fno-contracts-prove-override -fno-contracts-prove-assume" }
// { dg-additional-options "-fdump-ipa-contract_shadow_clone -fdump-tree-gimple" }

struct B {
  virtual int f (int x) pre (x > 0) post (r: r > 0);	// { dg-message "the predicate of the contract assertion" }
  virtual int g (int x) pre (x > 0) post (r: r > 0);
  virtual int h (int x) pre (x > 0);
};

struct D : B {
  // Proven in an override shadow, from B::f's precondition.
  int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
  { return x; }
  // Proven in D::g's proof shadow.
  int g (int x) override	// { dg-bogus "(cannot be proven|always false)" }
  { return x > 0 ? x : 1; }
  // A tautology, proven by folding.
  int h (int x) override pre (true) { return x; }	// { dg-bogus "(cannot be proven|always false)" }
};

void use () { D d; }

// Proven in k's proof shadow; an assumption under -fcontracts-prove-assume.
int k (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-bogus "(cannot be proven|always false)" }
  return i;
}

// { dg-final { scan-ipa-dump-not "Override shadow" "contract_shadow_clone" } }
// { dg-final { scan-tree-dump-not "CONTRACT_PROVEN" "gimple" } }

// The same diagnostics in a -O0 build, which proves at the proof level too.
// { dg-final { contracts-same-diagnostics "" "-O0" } }
