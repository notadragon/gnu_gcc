// DECISIONS.md E199 (b), E203: a proof through a premise copy is
// conditional, so under -fcontracts-prove-assume it is never assumed in the
// real code, while a sound proof next to it still is, exactly as without
// the copies: the ordinary shadows are decided first and their sound proofs
// stand.  h's evaluation is proven only through idx's observed
// postcondition (in the premise shadow); s's is proven soundly by its own
// test.  Only the postcondition is observed (a check with observe in s
// would make s's proof conditional too).
//
// Mirror: clang/test/Contracts/p4405-shadow-callee-premises-not-assumed.cpp
// in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontracts-prove-assume -fdump-tree-contract_proven -fdump-tree-contract_shadow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-shadow-callee-premises-not-assumed.json" }

#include <contracts>
using namespace std::contracts::labels;

int sink (int);

int idx (int x) post (r : r == 0) { return x; }

int h (int x)
{
  int i = idx (x);
  contract_assert<prove> (i == 0);	// { dg-bogus "cannot be proven" }
  return sink (i);
}

int s (int x)
{
  if (x != 0)
    return 0;
  contract_assert<prove> (x == 0);
  return sink (x);
}

// { dg-final { scan-tree-dump {contract obligation 0 \(line 27\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {callee premises: obligation 0 \(line 27\): proven, conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 1 \(line 35\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 0: not assumed} "contract_proven" } }
// { dg-final { scan-tree-dump {contract obligation 1: assumed} "contract_proven" } }
