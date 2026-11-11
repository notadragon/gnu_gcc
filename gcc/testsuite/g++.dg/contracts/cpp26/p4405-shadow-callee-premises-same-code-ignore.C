// DECISIONS.md E199 (b), E203 (A): the premise copies and premise shadows
// change nothing in the output.  With and without
// -fcontracts-prove-callee-premises the object code is the same, and so it is
// under -fcontracts-prove-assume: the ordinary shadows are decided as
// without the copies, a proof through a copy is never assumed, and s's sound
// proof is assumed either way.  The copies and premise shadows are
// analysis-only code, which IPA's decisions about the real code ignore, and
// are gone before anything is emitted.
// ignore: h3 is proven only through the copy of idx2, whose ignored
// contract_assert also calls only_in_premise, which nothing else calls.
//
// Mirror: clang/test/Contracts/p4405-shadow-callee-premises-same-code.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore -Wno-contract-unproven" }

#include <contracts>
using namespace std::contracts::labels;

int sink (int);
static int only_in_premise (int x) { return x * 7 + 1; }

int idx (int x) post (r : r == 0) { return x; }
int h1 (int x)
{
  int i = idx (x);
  contract_assert<diagnose> (i == 0);
  return sink (i);
}

inline int idx2 (int x)
{
  contract_assert (only_in_premise (x) == 1);
  contract_assert (x == 0);
  return x + sink (x);
}
int h3 (int x)
{
  int i = idx2 (x);
  contract_assert<diagnose> (i == 0);
  return sink (i);
}

int k (int x) { return idx (x) + 1; }
int h4 (int x)
{
  int i = k (x);
  contract_assert<diagnose> (i == 1);
  return sink (i);
}

void take (int x) pre (x == 0) { sink (x); }
int h5 (int x)
{
  take (x);
  contract_assert<diagnose> (x == 0);
  return sink (x + 1);
}

int s (int x)
{
  if (x != 0)
    return 0;
  contract_assert<diagnose> (x == 0);
  return sink (x / 3);
}

// { dg-final { contracts-same-object-code "-fno-contracts-prove-callee-premises -fno-contracts-prove-assume" "-fno-contracts-prove-assume" } }
// { dg-final { contracts-same-object-code "-fno-contracts-prove-callee-premises -fcontracts-prove-assume" "-fcontracts-prove-assume" } }
