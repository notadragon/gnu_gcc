// Test for GCC-896, fixed by marking functions that hold a contract assertion
// as refinable bodies, also in C: as refinable-body-ignore-replaceable-dtor.C,
// for a C99 inline function in C code compiled with -fexceptions, unwound by a
// C++ violation handler.
//
// refinable-body-ignore-replaceable-c99-i.c (C) has the inline definition of
// f with its assertion ignored, and use_cleanup, whose local c has a
// cleanup.  -fno-inline keeps the call out of line; the C99 rule sends it
// to the external definition in -c99-e.c (assertion enforced).  The
// middle end found f nothrow from the inline definition and dropped c's
// cleanup; now the C front end marks f, so it does not.  The violation
// handler (C++, shared with C code) throws: main catches the exception, and
// the cleanup must have run.  C has no throwing handler of its own, so this
// needs the mixed program; -x c compiles the two C files as C.
//
// Mirror: clang/test/Contracts/Runnable/ignore-ipa-replaceable-c99.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4299 -fexceptions -O2 -fno-inline" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable.json" }
// { dg-additional-options "-x c ${srcdir}/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-c99-e.c -x c ${srcdir}/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-c99-i.c -x none" }
// { dg-prune-output "is valid for C\\+\\+/ObjC\\+\\+ but not for C" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  throw 42;
}

extern "C" int use_cleanup (int);
extern "C" int cleanups;
int cleanups;

int main ()
{
  try
    {
      use_cleanup (-5);
    }
  catch (int)
    {
      if (cleanups != 1)
	__builtin_abort ();
      return 0;
    }
  __builtin_abort ();
}
