// P4405, DECISIONS.md E169: the dependence is recursive, through the
// predicates of the nested preconditions: the outer assertion calls g,
// whose precondition calls k, whose precondition is not proven when
// nothing is known of y -- the outer is unproven, noting k's precondition
// (g's own is proven by the proof shadow, k returning 1).  After a guard
// everything is proven.  A precondition calling its own function is not
// made again on the way.  At -O0: p4405-nested-recursive-O0.C.
//
// Mirror: clang/test/Contracts/p4405-nested-recursive.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }

int k (int x) pre (x > 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int k (int) { return 1; }
int g (int x) pre (k (x) == 1);	// { dg-bogus "precondition of a call" }
int g (int) { return 1; }

bool p (int x) pre (x == 0 || p (x - 1));

int
unprovable (int y)
{
  contract_assert (g (y) == 1);	// { dg-error "contract assertion cannot be proven" }
  return y;
}

int
guarded (int y)
{
  if (y <= 0)
    return 0;
  contract_assert (g (y) == 1);	// { dg-bogus "cannot be proven" }
  return y;
}

int
self (int y)
{
  contract_assert (p (y));	// { dg-error "contract assertion cannot be proven" }
  return y;
}

// A -O0 build, at the default proof level, 2, gives the same diagnostics.
// { dg-final { contracts-same-diagnostics "" "-O0" } }
