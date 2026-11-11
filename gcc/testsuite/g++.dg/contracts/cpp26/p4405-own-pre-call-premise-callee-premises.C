// GCC-1003 with -fcontracts-prove-callee-premises (implied by
// -fcontracts-p4405): an observe precondition whose predicate calls a
// function is a premise in its function's premise copy as well, so a
// caller's shadow that calls the copy proves conditionally what only that
// precondition gives.  take1's predicate calls an inline accessor, take2's
// an opaque function; h1's and h2's evaluations are left unproven by the
// ordinary shadow and proven, conditionally, by the premise shadow, and
// are never assumed.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe -fcontracts-prove-assume" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven" }

#include <contracts>
using namespace std::contracts::labels;

bool opaque (int);

struct C {
  int n;
  int get () const { return n; }
};

void take1 (const C &c) pre (c.get () > 0) {}
void h1 (C &c)
{
  take1 (c);
  contract_assert<prove> (c.n > 0);	// { dg-bogus "cannot be proven" }
}

void take2 (int x) pre (opaque (x) && x > 0) {}
void h2 (int x)
{
  take2 (x);
  contract_assert<prove> (x > 0);	// { dg-bogus "cannot be proven" }
}

// { dg-final { scan-tree-dump {contract obligation [0-9]+ \(line 28\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {callee premises: obligation [0-9]+ \(line 28\): proven, conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation [0-9]+ \(line 35\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {callee premises: obligation [0-9]+ \(line 35\): proven, conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump-not {: assumed} "contract_proven" } }
