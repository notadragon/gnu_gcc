// D4405R0, "Virtual Functions": no override proof is ever an assumption in
// the real code (-fcontracts-prove-assume, implied by -fcontracts-p4405):
// the overrider also serves calls that never established the overridden
// function's preconditions.  D1::f's precondition, whose runtime
// evaluation is ignore, is proven soundly from B1::f's, whose label
// settles it (DECISIONS.md E151), yet the real D1::f keeps its test of x.
// An ordinary sound proof next to an override shadow is still assumed:
// D2::f's contract_assert, which D2::f's override shadow takes as a
// premise.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -O2" }
// { dg-additional-options "-fdump-tree-optimized -fdump-tree-contract_shadow -fdump-tree-contract_proven" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int sink (int);

struct B1 { virtual int f (int x) pre<std::contracts::labels::always_enforce> (x > 0); };
struct D1 : B1 {
  virtual void key ();
  int f (int x) override
    pre (x >= 1)
  { return x >= 1 ? x : sink (x); }
};

struct B2 { virtual int f (int x) pre (x > 0); };
struct D2 : B2 {
  virtual void key ();
  int f (int x) override
    pre (x >= 1)
  {
    if (x < 5)
      return 0;
    contract_assert (x >= 5);
    return x;
  }
};

// The key functions emit the vtables, and with them D1::f and D2::f.
void D1::key () {}
void D2::key () {}

// { dg-final { scan-tree-dump {\(line 23, override pair [0-9]+\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 31, override pair [0-9]+\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 35\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump-times {: assumed} 1 "contract_proven" } }
// { dg-final { scan-tree-dump-not {: not assumed} "contract_proven" } }
// { dg-final { scan-tree-dump {sink \(x} "optimized" } }
