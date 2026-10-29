// P4405 premises decided by this TU (T1-GCC), across translation units.  g's
// shadow inlines the inline function h, whose precondition the TU of g
// enforces, but g calls h out of line (the parameters provoke that, as in
// p4405-premise-shadow-only.C), and another TU's h ignores it.  Whichever copy
// of h the linker keeps, the program must behave as written: g's diagnose
// evaluation relying on h's precondition is only conditionally proven, so
// nothing is assumed of it.  The linker keeps the first copy it sees: the
// enforcing one, so g (1, -5) is a violation, which the handler ends
// successfully.
//
// Mirror: clang/test/Contracts/Runnable/p4405-premise-multi-tu-shadow-only.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "--param large-function-insns=40 --param large-function-growth=10" }
// { dg-additional-options "--param max-inline-insns-single=200 --param max-inline-insns-auto=200 -fno-partial-inlining" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-multi-tu-shadow-only.json" }
// { dg-additional-sources "p4405-premise-multi-tu-shadow-only-e.cc p4405-premise-multi-tu-shadow-only-i.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>

int g (int, int);

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

int main ()
{
  g (1, -5);
  __builtin_abort ();
}

