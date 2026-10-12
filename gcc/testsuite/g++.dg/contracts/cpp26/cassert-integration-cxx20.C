// Under -fcontracts-p3290 the assert integration works from C++20, where
// __cpp_lib_assert_can_use_contracts is defined: a failed assert reaches the
// contract-violation handler.
//
// Mirror: clang/test/Contracts/cassert-integration-cxx20.cpp in the
// llvm_llvm-project fork.
// { dg-do run }
// { dg-additional-options "-std=c++20 -fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <version>
#define __STDC_WANT_ASSERT_USES_CONTRACTS__ 1
#include <cassert>
#include <contracts>
#include <cstdlib>

#ifndef __cpp_lib_assert_can_use_contracts
#error "__cpp_lib_assert_can_use_contracts not defined"
#endif

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.kind () == std::contracts::assertion_kind::cassert)
    std::_Exit (0);
}

int main ()
{
  int zero = 0;
  assert (zero == 1);
  return 1;
}
