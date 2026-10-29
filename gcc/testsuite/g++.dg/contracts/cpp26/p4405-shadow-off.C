// P4405 stage 3: -fno-contracts-prove-shadow switches the proof shadow
// off, on either side of -fcontracts-p4405, so at -O2 only folding runs and
// the evaluations p4405-shadow-prove.C proves are diagnosed again, each
// once.
//
// Mirror: clang/test/Contracts/p4405-shadow-off.cpp in the llvm_llvm-project
// fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fno-contracts-prove-shadow -fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove -fmax-errors=2" }

int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-error "contract assertion cannot be proven" }
  int c = n < 0 ? 0 : 1;
  contract_assert (c >= 0);	// { dg-error "contract assertion cannot be proven" }
  return c;
}
