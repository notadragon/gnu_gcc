// P4405: the umbrella -fcontracts-p3850 implies -fcontracts-p4405, which
// implies -fcontracts.
//
// Mirror: clang/test/Contracts/p4405-umbrella.cpp in the llvm_llvm-project
// fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3850 -fcontract-evaluation-semantic=prove" }

#ifndef __cpp_contracts_prove
#error "__cpp_contracts_prove not defined with -fcontracts-p3850"
#endif
#if __cpp_contracts_prove <= 0
#error "__cpp_contracts_prove not positive"
#endif

int f (int x) pre (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int g (int y) { return f (y); }		// { dg-error "contract assertion cannot be proven" }
