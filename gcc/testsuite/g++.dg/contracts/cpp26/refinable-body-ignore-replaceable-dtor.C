// Test for GCC-896, fixed by marking functions that hold a contract assertion
// as refinable bodies: as refinable-body-ignore-replaceable-catch.C, with a
// local whose destructor must run while the exception propagates.
//
// refinable-body-ignore-replaceable-i.cc's use_dtor has a local D and
// calls the inline f, which that TU's copy (assertion ignored) shows cannot
// throw, so use_dtor gets no cleanup for d.  The linker keeps the copy in
// refinable-body-ignore-replaceable-e.cc (assertion enforced), whose
// violation handler throws: main catches the exception, but d's destructor
// never ran (dtors is 0, not 1).  -fno-ipa-sra as in -catch.C.
//
// Mirror: clang/test/Contracts/Runnable/ignore-ipa-replaceable-dtor.cpp
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

int use_dtor (int);
extern int dtors;

int main ()
{
  try
    {
      use_dtor (-5);
    }
  catch (int)
    {
      if (dtors != 1)
	__builtin_abort ();
      return 0;
    }
  __builtin_abort ();
}
