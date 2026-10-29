// P4405 stage 3: -fcontracts-prove-shadow switches the proof shadow back on
// after -fno-contracts-prove-shadow, and works with folding switched off.
//
// Mirror: clang/test/Contracts/p4405-shadow-explicit.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fno-contracts-prove-shadow -fcontracts-prove-shadow -fno-contracts-prove-fold -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove" }

int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-bogus "cannot be proven|always false" }
  contract_assert (1 + 1 == 2);	// { dg-bogus "cannot be proven|always false" }
  contract_assert (i < n);	// { dg-error "contract assertion cannot be proven" }
  return i;
}
