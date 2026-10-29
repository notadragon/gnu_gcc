// P4405: -Werror=contract-unproven makes the diagnose semantic's warning an
// error.
//
// Mirror: clang/test/Contracts/p4405-werror.cpp in the llvm_llvm-project
// fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Werror=contract-unproven" }
// { dg-prune-output "some warnings being treated as errors" }

int f (int x) pre (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int g (int y) { return f (y); }		// { dg-error "contract assertion cannot be proven \\\[-Werror=contract-unproven\\\]" }
