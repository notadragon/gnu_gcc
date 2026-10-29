// D4405R0, "Virtual Functions", with -fno-contracts-prove-shadow alone:
// the override shadows are a proving strategy of their own
// (-fcontracts-prove-override, DECISIONS.md E157), so D::f's precondition
// is still proven from B::f's in its override shadow, while an ordinary
// evaluation, D::f's contract_assert, is settled by folding alone.
// Without either strategy, see p4405-override-nooverride-noshadow.C.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2 -fno-contracts-prove-shadow" }
// { dg-additional-options "-fdump-ipa-contract_shadow_clone" }

struct B { virtual int f (int x) pre (x > 0); };

struct D : B {
  int f (int x) override	// { dg-bogus "for a call through" }
    pre (x >= 0)
  {
    contract_assert (x >= 0);	// { dg-error "contract assertion cannot be proven" }
    return x;
  }
};

struct E : B {
  int f (int x) override pre (true) { return x; }	// { dg-bogus "cannot be proven" }
};

void use () { D d; E e; }

// { dg-final { scan-ipa-dump-times "Override shadow" 1 "contract_shadow_clone" } }
