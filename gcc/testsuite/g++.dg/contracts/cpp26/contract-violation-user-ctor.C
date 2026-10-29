// std::contracts::contract_violation has "no user-accessible constructor"
// ([support.contract.violation]), so user code cannot fabricate a violation.
//
// Mirror: clang/test/Contracts/contract-violation-user-ctor.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

#include <contracts>

void f ()
{
  std::contracts::contract_violation v (nullptr);  // { dg-error "private" }
}
