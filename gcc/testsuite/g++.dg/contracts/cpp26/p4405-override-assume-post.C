// D4405R0, "Virtual Functions": no override proof of a postcondition is
// ever an assumption in the real code (-fcontracts-prove-assume, implied
// by -fcontracts-p4405).  D::f's postcondition is proven in its override
// shadow, from B::f's precondition, but not in D::f's ordinary shadow, so
// the real D::f keeps its check; B::f's postcondition, proven there too,
// has no check in D::f to drop.  An ordinary sound proof next to override
// shadows is still assumed: D::g's postcondition.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -O2" }
// { dg-additional-options "-fdump-tree-optimized -fdump-tree-contract_shadow -fdump-tree-contract_proven" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct B
{
  virtual int f (int x)
    pre<std::contracts::labels::always_enforce> (x > 0)
    post (r: r > 0);
  virtual int g (int x) pre<std::contracts::labels::always_enforce> (x > 0);
};

struct D : B {
  virtual void key ();
  int f (const int x) override	// { dg-bogus "for a call through" }
    post (r: r > 0)		// { dg-warning "contract assertion cannot be proven" }
  { return x; }
  int g (const int x) override	// { dg-bogus "cannot be proven" }
    post (r: r >= 0)
  { return x > 0 ? x : 0; }
};

// The key function emits the vtable, and with it D::f and D::g.
void D::key () {}

// D::f's postcondition (line 26) and B::f's (line 19) in D::f's override
// shadow, which relies on D::f's ordinary obligation, unproven.
// { dg-final { scan-tree-dump {\(line 26, override pair [0-9]+\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 19, override pair [0-9]+\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 26\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 29\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 29, override pair [0-9]+\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump-times {: assumed} 1 "contract_proven" } }
// { dg-final { scan-tree-dump "__cxa_contract_violation_post" "optimized" } }
