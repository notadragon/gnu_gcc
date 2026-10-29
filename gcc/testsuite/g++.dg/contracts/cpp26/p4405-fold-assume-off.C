// P4405: -fno-contracts-prove-assume is accepted and leaves folding as it
// was (p4405-fold-assume.C).
//
// Mirror: clang/test/Contracts/p4405-fold-assume-off.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fno-contracts-prove-assume -fcontract-evaluation-semantic=prove" }

void f (int x)
{
  contract_assert (true);	// { dg-bogus "cannot be proven|always false" }
  contract_assert (false);	// { dg-error "contract predicate is always false" }
  contract_assert (x > 0);	// { dg-error "contract assertion cannot be proven" }
}
