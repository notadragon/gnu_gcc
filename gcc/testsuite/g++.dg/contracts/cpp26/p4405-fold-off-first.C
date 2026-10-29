// P4405: -fno-contracts-prove-fold written before -fcontracts-p4405 still
// turns folding off (p4405-fold-off.C).
//
// Mirror: clang/test/Contracts/p4405-fold-off-first.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fno-contracts-prove-fold -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove" }

void f ()
{
  contract_assert (true);	// { dg-error "contract assertion cannot be proven" }
  contract_assert (false);	// { dg-error "contract assertion cannot be proven" }
}
