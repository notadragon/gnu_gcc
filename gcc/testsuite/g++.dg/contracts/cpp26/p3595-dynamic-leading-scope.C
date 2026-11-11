// A P3595 "C++"-linkage selector named "::my_sel": the leading "::" names the
// global namespace, so the check calls my_sel (), which says observe: the
// violation is reported and execution continues.  Other empty components
// are rejected (p3595-dynamic-bad-name.C).
//
// Mirror: clang/test/Contracts/p3595-dynamic-leading-scope.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-leading-scope.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

std::contracts::evaluation_semantic my_sel ()
{
  return std::contracts::evaluation_semantic::observe;
}

int violations;
void handle_contract_violation (const std::contracts::contract_violation&)
{
  ++violations;
}

void f (int x) pre (x > 0) {}

int main ()
{
  f (-1);
  if (violations != 1)
    __builtin_abort ();
}
