// P4405, DECISIONS.md E169: p4405-nested-recursive.C at proof level 0,
// folding alone: g's precondition is not proven either, and both are noted, also
// after the guard.
//
// Mirror: clang/test/Contracts/p4405-nested-recursive.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0 -fcontracts-prove-optimize=0" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }

int k (int x) pre (x > 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int k (int) { return 1; }
int g (int x) pre (k (x) == 1);	// { dg-message "precondition of a call in the predicate cannot be proven" }
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
  contract_assert (g (y) == 1);	// { dg-error "contract assertion cannot be proven" }
  return y;
}

int
self (int y)
{
  contract_assert (p (y));	// { dg-error "contract assertion cannot be proven" }
  return y;
}
