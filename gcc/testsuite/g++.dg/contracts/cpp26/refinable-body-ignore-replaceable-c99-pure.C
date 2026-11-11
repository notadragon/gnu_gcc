// Test for GCC-896, fixed by marking functions that hold a contract assertion
// as refinable bodies, also in C: as refinable-body-ignore-replaceable-pure.C,
// for a C99 inline function.
//
// refinable-body-ignore-replaceable-c99-pure-i.c (C) has the inline
// definition of f with its assertion ignored; it is found const and to
// store nothing, so use_g folds the load of g after the call to 1.
// -fno-inline keeps the call out of line, and the C99 rule sends it to the
// external definition in -c99-pure-e.c (assertion observed), whose
// violation handler sets g to 5.  use_g (-5) must be 0; GCC gave -4.  The
// handler is C++ here only to share the harness: a C handler shows the
// same.  -x c compiles the two C files as C.
//
// Mirror: clang/test/Contracts/Runnable/ignore-ipa-replaceable-c99-pure.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4299 -O2 -fno-inline" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable.json" }
// { dg-additional-options "-x c ${srcdir}/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-c99-pure-e.c -x c ${srcdir}/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-c99-pure-i.c -x none" }
// { dg-prune-output "is valid for C\\+\\+/ObjC\\+\\+ but not for C" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

extern "C" int g;
int handled;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
  g = 5;
}

extern "C" int use_g (int);

int main ()
{
  int r = use_g (-5);
  if (handled != 1 || r != 0)
    __builtin_abort ();
}
