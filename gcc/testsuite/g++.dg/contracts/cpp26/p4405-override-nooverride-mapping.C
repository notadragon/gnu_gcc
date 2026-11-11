// D4405R0, "Virtual Functions", without override shadows
// (-fno-contracts-prove-override, DECISIONS.md E157): B::f's
// postconditions, obligations of D::f's own proven in its ordinary shadow,
// see what a call through B::f sees -- `this' the B subobject (here not
// the primary base, so adjusted), the result converted to B::f's return
// type for a covariant one, a reference parameter the object it refers
// to, and a by-value parameter as it was at entry, since the P3097
// wrapper checks the postcondition with its own copy, which D::f cannot
// modify (DECISIONS.md E153).  p4405-override-covariant.C and
// p4405-override-param.C test the same mapping in override shadows.
// Mirror: clang/test/Contracts/p4405-override-nooverride-mapping.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fcontract-analysis-semantic=prove -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-ipa-contract_shadow_clone -fdump-tree-optimized" }

struct Pad { int pad[4]; virtual void p (); };

struct B {
  int m;
  virtual B *self () post (r: r == this);	// { dg-message "the predicate of the contract assertion" }
  virtual int get (const int x) post (r: r == x);	// { dg-message "the predicate of the contract assertion" }
  virtual int member () post (r: r == m);
  virtual int &ref (int &x) post (r: &r == &x);
};

struct D : Pad, B {
  D *self () override { return this; }				// { dg-bogus "cannot be proven" }
  int get (int x) override { int r = x; x = 0; return r; }	// { dg-bogus "cannot be proven" }
  int member () override { return m; }				// { dg-bogus "cannot be proven" }
  int &ref (int &x) override { return x; }			// { dg-bogus "cannot be proven" }
};

struct E;
E *other ();

struct E : Pad, B {
  E *self () override		// { dg-error "cannot be proven for a call through 'virtual B\\* B::self\\(\\)'" }
  { return other (); }
  int get (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::get\\(int\\)'" }
  { x = 0; return x; }
};

void use () { D d; E e; }

// No override shadow: each pair's obligation is D::f's (E::f's) own, in
// its ordinary shadow.
// { dg-final { scan-ipa-dump-not "Override shadow" "contract_shadow_clone" } }
// { dg-final { scan-tree-dump-times {\(line 20, override pair [0-9]+ in the ordinary shadow\): sound} 1 "contract_shadow" } }
// { dg-final { scan-tree-dump-times {\(line 20, override pair [0-9]+ in the ordinary shadow\): unproven} 1 "contract_shadow" } }
// { dg-final { scan-tree-dump-times {\(line 21, override pair [0-9]+ in the ordinary shadow\): sound} 1 "contract_shadow" } }
// { dg-final { scan-tree-dump-times {\(line 21, override pair [0-9]+ in the ordinary shadow\): unproven} 1 "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 22, override pair [0-9]+ in the ordinary shadow\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 23, override pair [0-9]+ in the ordinary shadow\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump-not "__contract_override" "optimized" } }
