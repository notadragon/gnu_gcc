// Test for GCC-896, fixed by marking functions that hold a contract assertion
// as refinable bodies: as refinable-body-ignore-replaceable-dtor.C, with
// "cannot throw" derived by the middle end instead of the front end.
//
// In refinable-body-ignore-replaceable-late-i.cc, f calls the inline h,
// defined after f, so the front end does not mark f TREE_NOTHROW (the call
// to h might throw when f is finished).  The middle end still finds f
// nothrow from that TU's copy (assertion ignored): pass_nothrow and
// pure-const at -O2, and set_nothrow_function_flags on f's RTL at -O0 when
// f is expanded before its caller; either way use_dtor's cleanup of d is
// removed.  The linker keeps the copy in -late-e.cc (assertion enforced),
// whose violation handler throws: main catches the exception, but d's
// destructor never ran.  So fixing only the front end's marking
// (cp/decl.cc) does not fix this test.  -fno-ipa-sra as in -catch.C.
//
// Mirror: clang/test/Contracts/Runnable/ignore-ipa-replaceable-late.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -O2 -fno-ipa-sra" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable.json" }
// { dg-additional-sources "refinable-body-ignore-replaceable-late-e.cc refinable-body-ignore-replaceable-late-i.cc" }
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
