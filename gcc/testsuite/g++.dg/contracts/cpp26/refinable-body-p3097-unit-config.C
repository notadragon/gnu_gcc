// DECISIONS.md E136, the P3097 shape: whether a virtual call's interface
// contracts are checked outside the callee is decided per translation
// unit, by the semantic each unit's configuration gives them.
// refinable-body-p3097-unit-config-k.cc, linked first, holds the
// definitions of g and ng the linker keeps, which call B::f and B::nf
// through P3097 wrappers checking their preconditions with enforce.
// refinable-body-p3097-unit-config-u.cc ignores the preconditions, and its
// callers are compiled with its own g and ng.  Unlike the caller-side form
// (open-bug-caller-side-unwrapped.C, GCC-904), this cannot go wrong: u's ng
// cannot throw only because B::nf is noexcept, and then the kept wrapper's
// throwing handler terminates too (p3097-noexcept-caller-side-terminate.C);
// u's g calls a function that may throw, so use_g keeps its handler and
// catches the exception from the kept g's wrapper.
//
// -fno-ipa-sra: otherwise the callers could call a local clone of their own
// translation unit's definitions, which would hide a problem.
//
// Mirror: in the llvm_llvm-project fork,
// clang/test/Contracts/Runnable/refinable-body-p3097-unit-config.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -O2 -fno-ipa-sra" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-body-p3097-unit-config.json" }
// { dg-additional-sources "refinable-body-p3097-unit-config-k.cc refinable-body-p3097-unit-config-u.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <exception>

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  throw 42;
}

int use_g (int);
int use_ng (int);

static void
on_terminate ()
{
  std::_Exit (0);
}

int
main ()
{
  if (use_g (-5) != -1)		// caught in use_g
    std::abort ();
  // A throwing handler under a noexcept function terminates, whichever
  // definition of ng runs.
  std::set_terminate (on_terminate);
  use_ng (-5);
  std::abort ();
}
