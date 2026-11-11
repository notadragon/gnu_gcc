// A handler defined noexcept, which [basic.contract.handler] explicitly
// permits, is accepted: <contracts> declares no handler.
//
// Mirror: clang/test/Contracts/noexcept-violation-handler.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

void handle_contract_violation (const std::contracts::contract_violation&) noexcept {}

int main () {}
