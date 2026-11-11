// P3595: a "C++"-linkage selector name is looked up as a qualified name, so
// "lib::my_sel" finds a selector declared in an inline namespace lib::v1,
// and "lib::tagged" one declared with an abi_tag, and the check calls that
// function rather than a synthesized lib::my_sel () / lib::tagged ().  With
// "provideweak", calling the synthesized weak default (enforce) would
// abort; the user's selectors say observe, so both violations continue.
//
// Mirror: clang/test/Contracts/p3595-dynamic-inline-namespace.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-inline-namespace.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace lib {
inline namespace v1 {
std::contracts::evaluation_semantic my_sel ()
{
  return std::contracts::evaluation_semantic::observe;
}
}
[[gnu::abi_tag ("x")]] std::contracts::evaluation_semantic tagged ()
{
  return std::contracts::evaluation_semantic::observe;
}
}

int violations;

void handle_contract_violation (const std::contracts::contract_violation&)
{
  ++violations;
}

namespace n1 { void f (int x) pre (x > 0) {} }
namespace n2 { void f (int x) pre (x > 0) {} }

int main ()
{
  n1::f (-1);
  n2::f (-1);
  if (violations != 2)
    __builtin_abort ();
}
