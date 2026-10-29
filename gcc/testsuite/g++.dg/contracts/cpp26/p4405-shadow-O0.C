// P4405 stage 3: at -O0 there is no proof shadow, so folding is final: the
// evaluations a shadow would prove at -O2 are diagnosed, each once, and
// one that folds is still proven.  A function's own optimize attribute
// decides for it (p4405-shadow-O0-attribute.C, -O2-attribute.C).
//
// Mirror: clang/test/Contracts/p4405-shadow-O0.cpp in the llvm_llvm-project
// fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove -fmax-errors=2" }

int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-error "contract assertion cannot be proven" }
  contract_assert (2 > 1);	// { dg-bogus "cannot be proven|always false" }
  int c = n < 0 ? 0 : 1;
  contract_assert (c >= 0);	// { dg-error "contract assertion cannot be proven" }
  return c;
}
