// D4405's proof optimization level (DECISIONS.md E162-E168, E205):
// -fcontracts-prove-optimize=0 at -O2: folding only, so D::h alone is
// proven; the override pairs take the -fno-contracts-prove-override
// fallback (E157).
//
// Each strategy is on from a proof level (fold 0, shadow and assume 1,
// override 2): D::f is proven by an override shadow, D::g and k by
// their proof shadows, D::h by folding, and k's evaluation becomes an
// assumption (CONTRACT_PROVEN in the gimple dump) with
// -fcontracts-prove-assume.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2 -fcontracts-prove-optimize=0" }
// { dg-additional-options "-fdump-ipa-contract_shadow_clone -fdump-tree-gimple" }

struct B {
  virtual int f (int x) pre (x > 0) post (r: r > 0);	// { dg-message "the predicate of the contract assertion" }
  virtual int g (int x) pre (x > 0) post (r: r > 0);	// { dg-message "the predicate of the contract assertion" }
  virtual int h (int x) pre (x > 0);
};

struct D : B {
  // Proven in an override shadow, from B::f's precondition.
  int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
  { return x; }
  // Proven in D::g's proof shadow.
  int g (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::g\\(int\\)'" }
  { return x > 0 ? x : 1; }
  // A tautology, proven by folding.
  int h (int x) override pre (true) { return x; }	// { dg-bogus "(cannot be proven|always false)" }
};

void use () { D d; }

// Proven in k's proof shadow; an assumption under -fcontracts-prove-assume.
int k (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-error "contract assertion cannot be proven" }
    // { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-1 }
  return i;
}

// { dg-final { scan-tree-dump-not "CONTRACT_PROVEN" "gimple" } }

// The same diagnostics in a -O0 build, which proves at the proof level too.
// { dg-final { contracts-same-diagnostics "" "-O0" } }
