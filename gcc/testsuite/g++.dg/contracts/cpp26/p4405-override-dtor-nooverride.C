// D4405R0, "Virtual Functions", DECISIONS.md E152 and E157: without
// override shadows (-fno-contracts-prove-override), a virtual destructor --
// an implicitly defined one too -- must establish the overridden
// destructors' postconditions as obligations of its own, proven in its
// ordinary shadow.  In a class with virtual bases only the complete-object
// destructor holds them.  The diagnostics still name the overridden
// destructor.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=observe -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-ipa-contract_shadow_clone -fdump-tree-optimized" }

int g;

struct B {
  virtual ~B () post (g == 1) { g = 1; }
};

struct D : B {			// { dg-bogus "cannot be proven" }
};

int g2;

struct B2 {
  virtual ~B2 () post (g2 == 1);	// { dg-message "the predicate of the contract assertion" }
};

struct E : B2 {
  ~E () override {}		// { dg-error "cannot be proven for a call through 'virtual B2::~B2\\(\\)'" }
};

struct F : B2 {			// { dg-error "cannot be proven for a call through 'virtual B2::~B2\\(\\)'" }
};

struct V : virtual B {		// { dg-bogus "cannot be proven" }
};

void use () { D d; E e; F f; V v; }

// { dg-final { scan-ipa-dump-not "Override shadow" "contract_shadow_clone" } }
// { dg-final { scan-tree-dump-times {\(line 15, override pair [0-9]+ in the ordinary shadow\): sound} 2 "contract_shadow" } }
// { dg-final { scan-tree-dump-times {\(line 24, override pair [0-9]+ in the ordinary shadow\): unproven} 2 "contract_shadow" } }
// { dg-final { scan-tree-dump-not "__contract_override" "optimized" } }
// { dg-final { scan-tree-dump-not "\\.CONTRACT_" "optimized" } }
