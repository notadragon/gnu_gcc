// D4405R0, "Virtual Functions", DECISIONS.md E151: a proof in an override
// shadow relies on the overridden function's preconditions, which every
// caller evaluates -- in any translation unit, with that unit's semantic
// -- so they are guaranteed only when their labels admit nothing but
// terminating semantics and assume.  D::g's precondition is then proven
// soundly, D::f's only conditionally; neither is diagnosed.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct B {
  virtual int f (int x) pre (x > 0);
  virtual int g (int x) pre<std::contracts::labels::always_enforce> (x > 0);
};

struct D : B {
  int f (int x) override pre (x >= 1) { return x; }	// { dg-bogus "cannot be proven" }
  int g (int x) override pre (x >= 1) { return x; }	// { dg-bogus "cannot be proven" }
};

void use () { D d; }

// { dg-final { scan-tree-dump {\(line 20, override pair [0-9]+\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 21, override pair [0-9]+\): sound} "contract_shadow" } }
