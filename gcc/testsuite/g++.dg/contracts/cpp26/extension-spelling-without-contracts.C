// With contracts off, the extension spellings __pre, __post and
// __contract_assert are diagnosed, without a crash.
//
// Mirror: clang/test/Contracts/extension-pre-without-contracts.cpp and
// extension-contract-assert-without-contracts.cpp in the llvm_llvm-project
// fork.
// { dg-do compile }
// { dg-options "-std=c++17" }

void f (int x) __pre (x > 0) {}			// { dg-error "expected initializer" }
void h (int x) __post (x > 0) {}		// { dg-error "expected initializer" }
void g (int x) { __contract_assert (x > 0); }	// { dg-error "only available with '-fcontracts'" }
