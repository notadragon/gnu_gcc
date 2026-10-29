// Test for GCC-896, fixed by marking functions that hold a contract assertion
// as refinable bodies: GCC derives "reads and writes no memory" from the body
// of a function another translation unit may provide, so a definition linked
// from a TU that evaluates the function's contract assertion, calling a
// violation handler with side effects, has effects its callers were compiled
// not to expect.
//
// refinable-body-ignore-replaceable-pure-i.cc defines the inline f with its
// assertion ignored: that copy is found const (pure-const) and its mod-ref
// summary says it stores nothing, so use_g, compiled there, folds the load
// of g after the call to the 1 it stored before it.  The linker keeps the
// copy in -pure-e.cc, which observes the assertion: f (-5) calls the
// handler, which sets g to 5, and returns -5.  use_g (-5) must be 0
// (-5 + 5); GCC gave -4.  The handler did run (handled is 1), so no
// choice of evaluation semantic gives -4.  Either pure-const or mod-ref
// alone is enough to fold (-fno-ipa-pure-const and -fno-ipa-modref each
// leave the other doing it); -fno-ipa-sra as in -catch.C.
//
// Mirror: clang/test/Contracts/Runnable/ignore-ipa-replaceable-pure.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -O2 -fno-ipa-sra" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable.json" }
// { dg-additional-sources "refinable-body-ignore-replaceable-pure-e.cc refinable-body-ignore-replaceable-pure-i.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

extern int g;
int handled;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
  g = 5;
}

int use_g (int);

int main ()
{
  int r = use_g (-5);
  if (handled != 1 || r != 0)
    __builtin_abort ();
}
