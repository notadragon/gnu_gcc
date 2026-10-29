// The refinable-body rule for contract wrappers: a function that calls a
// caller-side wrapper (P3595) or a P3097 wrapper for a virtual call holds
// the evaluations the wrapper makes, which another translation unit's
// definition of the function may make differently or not at all, so it is
// a refinable body whether or not the wrapper is inlined into it.
//
// refinable-fork-wrapper-u.cc holds the callers and the definitions of g
// and h they are compiled with: g checks f's postcondition caller-side,
// with enforce, and h's P3097 wrapper checks vf's postcondition with
// enforce, so each wrapper's return value -- and so g's and h's -- is
// positive.  -fno-inline keeps the wrappers out of line, so nothing but the
// call of the wrapper marks g and h.  refinable-fork-wrapper-k.cc, linked
// first, holds the definitions of g and h the linker keeps, which check
// nothing.  use_g (-5) and use_h (bp, -5) must be 0.
//
// -fno-ipa-sra: otherwise the callers call a local clone of their own
// translation unit's definition, which hides the problem.
//
// Mirror: clang/test/Contracts/Runnable/refinable-fork-wrapper.cpp (the P3097
// half; Clang has no caller-side checking)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -O2 -fno-inline -fno-ipa-sra" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-fork-wrapper.json" }
// { dg-additional-sources "refinable-fork-wrapper-k.cc refinable-fork-wrapper-u.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  __builtin_abort ();
}

struct B;
extern B *bp;
int use_g (int);
int use_h (B *, int);

int
main ()
{
  if (use_g (-5) != 0 || use_h (bp, -5) != 0)
    __builtin_abort ();
}
