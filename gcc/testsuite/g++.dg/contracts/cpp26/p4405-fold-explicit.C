// P4405: -fcontracts-prove-fold written explicitly, after
// -fno-contracts-prove-fold, turns folding back on.
//
// Mirror: clang/test/Contracts/p4405-fold-explicit.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fno-contracts-prove-fold -fcontracts-prove-fold -fcontract-evaluation-semantic=prove" }

void f ()
{
  contract_assert (true);	// { dg-bogus "cannot be proven|always false" }
  contract_assert (false);	// { dg-error "contract predicate is always false" }
}
