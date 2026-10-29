// Test for GCC-896, fixed by marking functions that hold a contract assertion
// as refinable bodies: GCC derives "cannot throw" from the body of a function
// another translation unit may provide, so a definition linked from a TU that
// evaluates the function's contract assertion with a throwing violation handler
// throws through callers compiled as if nothing could.
//
// refinable-body-ignore-replaceable-i.cc defines the inline f with its
// assertion ignored; f's body then has no call, so the C++ front end marks
// f TREE_NOTHROW (finish_function: f is comdat, so decl_replaceable_p is
// false), and use_catch, compiled there, drops its handler.
// refinable-body-ignore-replaceable-e.cc defines the same f with the
// assertion enforced, and the linker keeps that copy (the first it sees).
// The violation handler throws, so use_catch (-5) must catch the exception
// and return -1; GCC called std::terminate instead.  Upstream GCC behaved
// the same with -fcontract-evaluation-semantic=ignore and =enforce (or
// =observe) for the two TUs.  -fno-ipa-sra: otherwise use_catch could
// call a local clone of its own TU's f, which would hide the problem.
// refinable-body-ignore-replaceable-late.C: the same, with the facts
// coming from the middle end rather than the front end.
//
// Mirror: clang/test/Contracts/Runnable/ignore-ipa-replaceable-catch.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -O2 -fno-ipa-sra" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable.json" }
// { dg-additional-sources "refinable-body-ignore-replaceable-e.cc refinable-body-ignore-replaceable-i.cc" }
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
