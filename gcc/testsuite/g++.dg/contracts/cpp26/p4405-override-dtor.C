// D4405R0, "Virtual Functions", DECISIONS.md E152: a virtual destructor
// overrides the virtual destructors of its bases, and gets override shadows
// like any other overrider -- an implicitly defined one too.  The body
// analyzed is what the destructor runs: its own statements, then the
// destruction of its members and bases, so the overridden destructor's
// postconditions are proven after it has run.  One analysis per source
// destructor: the deleting destructor only calls the complete one, and in
// a class with virtual bases only the complete-object destructor, which is
// what a delete through a base class runs, holds the override regions (the
// base-object destructor does not destroy the virtual base).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=observe -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-ipa-contract_shadow_clone" }

int g;

struct B {
  virtual ~B () post (g == 1);	// { dg-error "contract assertion cannot be proven" }
  // { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-1 }
};

B::~B () {}

// User-provided: proven from its own body.
struct D : B {
  ~D () override { g = 1; }	// { dg-bogus "cannot be proven" }
};

// User-provided: unproven.
struct E : B {
  ~E () override {}		// { dg-error "cannot be proven for a call through 'virtual B::~B\\(\\)'" }
};

// Implicitly defined: unproven, diagnosed at the class.
struct F : B {			// { dg-error "cannot be proven for a call through 'virtual B::~B\\(\\)'" }
  int y;
};

// Implicitly defined: proven by a member's destructor, which runs before
// B::~B.
struct Setter { ~Setter () { g = 1; } };
struct G : B {			// { dg-bogus "cannot be proven" }
  Setter s;
};

// An indirect base: H::~H overrides B::~B through G::~G.
struct H : G {			// { dg-bogus "cannot be proven" }
};

// A destructor's own precondition, proven at entry from nothing (B::~B
// has none: DECISIONS.md E156).
struct P : B {
  ~P () override pre (true) { g = 1; }	// { dg-bogus "cannot be proven" }
};

// A virtual base: the complete-object destructor runs B3::~B3, which
// establishes its postcondition itself; the base-object destructor does
// not, and is never the target of a delete through B3.
int g3;
struct B3 {
  virtual ~B3 () post (g3 == 1) { g3 = 1; }
};
struct V1 : virtual B3 {	// { dg-bogus "cannot be proven" }
};
struct V2 : virtual B3 {
  ~V2 () override { g3 = 2; }	// { dg-bogus "cannot be proven" }
};
struct W : V1, V2 {		// { dg-bogus "cannot be proven" }
};

void kill (B *p) { delete p; }
void kill3 (B3 *p) { delete p; }
void use () { D d; E e; F f; G gg; H h; P p; V1 v1; V2 v2; W w; }

// One override shadow per destructor (the complete-object one for V1, V2
// and W), none for the base-object or deleting destructors.
// { dg-final { scan-ipa-dump-times "Override shadow" 9 "contract_shadow_clone" } }
// { dg-final { scan-tree-dump-times {Function [^\n]*D1Ev\.contract_override} 3 "contract_shadow" } }
// { dg-final { scan-tree-dump-not {_ZN(2V1|2V2|1W)D2Ev\.contract_override} "contract_shadow" } }
// { dg-final { scan-tree-dump-not {D0Ev\.contract_override} "contract_shadow" } }
// { dg-final { scan-tree-dump-times {\(line 18, override pair [0-9]+\): unproven} 2 "contract_shadow" } }
// { dg-final { scan-tree-dump-times {\(line 18, override pair [0-9]+\): sound} 4 "contract_shadow" } }
// { dg-final { scan-tree-dump-times {\(line 61, override pair [0-9]+\): sound} 3 "contract_shadow" } }
