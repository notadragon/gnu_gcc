// P4405: -fno-contracts-prove-fold turns the folding strategy off, giving
// stage 1's behaviour back: every evaluation with prove is an error, "cannot
// be proven", even when its predicate is true or false.  The -fno- form
// wins whichever side of -fcontracts-p4405 it is written on
// (p4405-fold-off-first.C).
//
// Mirror: clang/test/Contracts/p4405-fold-off.cpp in the llvm_llvm-project
// fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fno-contracts-prove-fold -fcontract-evaluation-semantic=prove" }

void f ()
{
  contract_assert (true);	// { dg-error "contract assertion cannot be proven" }
  contract_assert (false);	// { dg-error "contract assertion cannot be proven" }
}
