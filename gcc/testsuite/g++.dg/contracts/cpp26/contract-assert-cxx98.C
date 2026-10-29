// Below C++11 contract_assert is an ordinary identifier, even with
// -fcontracts: a function of that name compiles.
//
// Mirror: clang/test/Contracts/contract-assert-cxx98.cpp in the
// llvm_llvm-project fork.
// { dg-do compile }
// { dg-options "-std=c++98 -fcontracts" }

int contract_assert (int);

void f (int x) { contract_assert (x > 0); }
