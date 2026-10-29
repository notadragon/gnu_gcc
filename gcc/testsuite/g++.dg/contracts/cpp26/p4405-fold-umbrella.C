// P4405: the umbrella -fcontracts-p3850 implies -fcontracts-p4405, and so
// the folding strategy.
//
// Mirror: clang/test/Contracts/p4405-fold-umbrella.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3850 -fcontract-evaluation-semantic=prove" }

void f (int x)
{
  contract_assert (true);	// { dg-bogus "cannot be proven|always false" }
  contract_assert (false);	// { dg-error "contract predicate is always false" }
  contract_assert (x > 0);	// { dg-error "contract assertion cannot be proven" }
}
