// D4405R0, "Virtual Functions": `final' changes nothing about an
// overrider's own override shadows (it only stops later overriders), and a
// postcondition with captures (P3098), of the overridden function or of the
// overrider, is decided at once in every override shadow, as its own
// analysis evaluation is: its captures are initialized only for a checked
// evaluation, so only folding can settle it.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p3098 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }
// { dg-additional-options "-fdump-ipa-contract_shadow_clone" }

int cap (int);

struct B {
  virtual int f (int x) post [c = cap (x)] (r: r >= c);	// { dg-message "the predicate of the contract assertion" }
  virtual int g (int x) pre (x > 0);
  virtual int h (int x) pre (x > 0) post [c = 1] (r: r >= c);	// { dg-message "the predicate of the contract assertion" }
};

struct D final : B {
  int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
  { return cap (x); }
  int g (int x) override final	// { dg-bogus "cannot be proven" }
    pre (x > -1)
  { return x; }
};

struct E : B {
  // Both postconditions have captures: E::h's own analysis evaluation
  // (an error under prove), and in the override shadow E::h's and B::h's
  // obligations, are each decided at once, unproven, although B::h's
  // precondition would give `r >= 1'.
  int h (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::h\\(int\\)'" }
    post [d = x] (r: r == d)	// { dg-error "contract assertion cannot be proven" }
  { return x; }			// { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-1 }
};

void use () { D d; E e; }

// D::g's precondition, proven from B::g's in its override shadow, is the
// only deferred obligation.
// { dg-final { scan-ipa-dump-times "Override shadow" 1 "contract_shadow_clone" } }
