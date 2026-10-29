// P4405 stage 3: -fno-contracts-prove-shadow after -fcontracts-p4405 also
// switches the proof shadow off (p4405-shadow-off.C has it first).
//
// Mirror: clang/test/Contracts/p4405-shadow-off-last.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fno-contracts-prove-shadow -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove -fmax-errors=1" }

int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-error "contract assertion cannot be proven" }
  return i;
}
