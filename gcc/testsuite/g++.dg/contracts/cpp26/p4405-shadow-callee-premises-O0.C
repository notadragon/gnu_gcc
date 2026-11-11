// DECISIONS.md E199 (b): at proof level 0 there are no proof shadows, so no
// premise copies either: the diagnostics and the object code are the same
// with and without -fcontracts-prove-callee-premises.
//
// Mirror: clang/test/Contracts/p4405-shadow-callee-premises-O0.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O0 -fcontracts-prove-optimize=0" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe -fcontracts-prove-callee-premises" }

#include <contracts>
using namespace std::contracts::labels;

int idx (int x) post (r : r == 0) { return x; }
void h1 (int x)
{
  int i = idx (x);
  contract_assert<diagnose> (i == 0);	// { dg-warning "cannot be proven" }
}

inline int idx2 (int x)
{
  contract_assert (x == 0);
  return x;
}
void h3 (int x)
{
  int i = idx2 (x);
  contract_assert<diagnose> (i == 0);	// { dg-warning "cannot be proven" }
}

void take (int x) pre (x == 0) {}
void h5 (int x)
{
  take (x);
  contract_assert<diagnose> (x == 0);	// { dg-warning "cannot be proven" }
}

// { dg-final { contracts-same-object-code "-fno-contracts-prove-callee-premises -Wno-contract-unproven" "-Wno-contract-unproven" } }
