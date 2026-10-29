// Test for GCC-896, fixed by marking functions that hold a contract assertion
// as refinable bodies: as refinable-body-ignore-replaceable-catch.C, for an
// implicit instantiation of a function template (comdat, like an inline
// function).
//
// refinable-body-ignore-replaceable-template-i.cc instantiates f<int>
// with the assertion ignored, finds it cannot throw, and drops
// use_catch's handler; the linker keeps the instantiation in -template-e.cc
// (assertion enforced), whose violation handler throws.  use_catch (-5)
// must return -1; GCC called std::terminate.  -fno-ipa-sra as in -catch.C.
//
// Mirror: clang/test/Contracts/Runnable/ignore-ipa-replaceable-template.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -O2 -fno-ipa-sra" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable.json" }
// { dg-additional-sources "refinable-body-ignore-replaceable-template-e.cc refinable-body-ignore-replaceable-template-i.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  throw 42;
}

int use_catch (int);

int main ()
{
  if (use_catch (-5) != -1)
    __builtin_abort ();
}
