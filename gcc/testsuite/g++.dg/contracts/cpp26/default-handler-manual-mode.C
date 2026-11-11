// The default violation handler reports a P3290 manual violation, whose
// detection mode is unspecified, as "mode: unspecified", not as a throwing
// predicate.
//
// Mirror: clang/test/Contracts/default-handler-manual-mode.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int main ()
{
  std::contracts::handle_observed_contract_violation ("manual check");
  return 0;
}

// { dg-output "manual check\[^\n\]*\n\\\[assertion_kind: manual, semantic: observe, mode: unspecified" }
