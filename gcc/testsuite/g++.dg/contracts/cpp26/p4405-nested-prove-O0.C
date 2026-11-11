// P4405, DECISIONS.md E169: p4405-nested-prove.C at proof level 0,
// where folding alone decides each evaluation, with the arguments of the nested calls
// substituted: the outer assertions that call g1, g2, g3 and g5 are not
// proven, each noting the nested precondition (always false for `g3 (0)');
// the nested precondition of `g4 (5)' is proven, so only the outer, which
// folding cannot decide, is reported.
//
// Mirror: clang/test/Contracts/p4405-nested-prove.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0 -fcontracts-prove-optimize=0" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }

int g1 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int g1 (int) { return 1; }
int g2 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int g2 (int) { return 1; }
int g3 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate is always false" }
int g3 (int x) { return 10 / x; }
int g4 (int x) pre (x != 0);	// { dg-bogus "precondition of a call" }
int g4 (int) { return 1; }
int g5 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int g5 (int x) { return 10 / x; }

int
unprovable (int y)
{
  contract_assert (g1 (y) == 1);	// { dg-error "contract assertion cannot be proven" }
  return y;
}

int
guarded (int y)
{
  if (y == 0)
    return 0;
  contract_assert (g2 (y) == 1);	// { dg-error "contract assertion cannot be proven" }
  return y;
}

int
always_false (int y)
{
  contract_assert (g3 (0) != 7);	// { dg-error "contract assertion cannot be proven" }
  return y;
}

int
guarded_zero (int y)
{
  if (y != 0)
    return 0;
  contract_assert (g5 (y) != 7);	// { dg-error "contract assertion cannot be proven" }
  return y;
}

int
provable (int y)
{
  contract_assert (g4 (5) == 1);	// { dg-error "contract assertion cannot be proven" }
  return y;
}
