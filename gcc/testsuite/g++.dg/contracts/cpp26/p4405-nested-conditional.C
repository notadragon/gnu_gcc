// P4405, DECISIONS.md E169: a nested precondition proven only
// conditionally -- here relying on an earlier assertion that is only
// observed -- makes the outer assertion conditional, so that
// -fcontracts-prove-assume does not turn it into an assumption.  Only the
// outer assertion and g's precondition are analyzed (with prove).
//
// Mirror: clang/test/Contracts/p4405-nested-conditional.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontracts-prove-assume" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-nested-conditional.json" }

namespace pp {
int g (int x) pre (x != 0);
int g (int) { return 1; }
}

int
f (int y)
{
  contract_assert (y != 0);
  contract_assert (pp::g (y) == 1);	// { dg-bogus "cannot be proven" }
  return y;
}

// { dg-final { scan-tree-dump {\(line 25\): nested in} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 25\), with those nested in it: conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump-not {\(line 25\)[^\n]*: sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {: not assumed} "contract_proven" } }
// { dg-final { scan-tree-dump-not {: assumed} "contract_proven" } }
