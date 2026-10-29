// P4405: -Wno-contract-unproven silences the diagnose semantic's warning.
//
// Mirror: clang/test/Contracts/p4405-wno-contract-unproven.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

int f (int x)
  pre (x > 0)			// { dg-bogus "cannot be proven" }
  post (r: r > 0)		// { dg-bogus "cannot be proven" }
{
  contract_assert (x != 3);	// { dg-bogus "cannot be proven" }
  return x;
}
