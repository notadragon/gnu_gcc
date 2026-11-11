// p4405-shadow-callee-premises-ignore.C with
// -fno-contracts-prove-callee-premises: h3's premise, in the inline callee,
// is lost too.
//
// Mirror: clang/test/Contracts/p4405-shadow-callee-premises.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore -fno-contracts-prove-callee-premises" }

#include <contracts>
using namespace std::contracts::labels;

int idx (int x) post (r : r == 0) { return x; }
void h1 (int x)
{
  int i = idx (x);
  contract_assert<prove> (i == 0);	// { dg-error "cannot be proven" }
}

void h2 (int x)
{
  contract_assert (x == 0);
  contract_assert<prove> (x == 0);	// { dg-bogus "cannot be proven" }
}

inline int idx2 (int x)
{
  contract_assert (x == 0);
  return x;
}
void h3 (int x)
{
  int i = idx2 (x);
  contract_assert<prove> (i == 0);	// { dg-error "cannot be proven" }
}

int k (int x) { return idx (x) + 1; }
void h4 (int x)
{
  int i = k (x);
  contract_assert<prove> (i == 1);	// { dg-error "cannot be proven" }
}

void take (int x) pre (x == 0) {}
void h5 (int x)
{
  take (x);
  contract_assert<prove> (x == 0);	// { dg-error "cannot be proven" }
}

