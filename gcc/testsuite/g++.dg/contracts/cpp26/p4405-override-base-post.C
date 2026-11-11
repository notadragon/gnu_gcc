// D4405R0, "Virtual Functions": the overridden function's postconditions
// are proven in each override shadow after all of the overrider's checks,
// with the overridden function's preconditions as premises, also for an
// overrider with no contract assertions of its own (which then gets the
// try/finally of a postcondition check, in its override shadow only).
// Unproven, one is diagnosed at the overrider's definition, naming the
// overridden function, with a note at the predicate.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow" }

struct B {
  virtual int f (int x) post (r: r > 0);	// { dg-message "the predicate of the contract assertion" }
  virtual int g (int x) pre (x > 0) post (r: r > 0);
  virtual int h (const int x) pre (x > 0) post (r: r >= x);
};

struct D : B {
  int f (int x) override	// { dg-error "contract assertion cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
  { return x; }
  int g (int x) override	// { dg-bogus "cannot be proven" }
  { return x; }
  int h (int x) override	// { dg-bogus "cannot be proven" }
  { return x > 5 ? x : x + 1; }
};

struct E : B {
  int f (int x) override	// { dg-bogus "cannot be proven" }
  { return x > 0 ? x : 1; }
};

// An overrider with a precondition and no postcondition, whose override
// shadow has the try/finally of B::g's postcondition's obligation.
struct F : B {
  int g (int x) override	// { dg-bogus "cannot be proven" }
    pre (x >= 1)
  { return x; }
};

void use () { D d; E e; F f; }

// B::f's postcondition (line 13) unproven in D::f's override shadow and
// proven soundly in E::f's (no premise used); B::g's and B::h's (lines 14
// and 15) proven conditionally from their preconditions, B::g's in D::g's
// and F::g's.
// { dg-final { scan-tree-dump {\(line 13, override pair [0-9]+\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 13, override pair [0-9]+\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump-times {\(line 14, override pair [0-9]+\): conditional} 2 "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 15, override pair [0-9]+\): conditional} "contract_shadow" } }
