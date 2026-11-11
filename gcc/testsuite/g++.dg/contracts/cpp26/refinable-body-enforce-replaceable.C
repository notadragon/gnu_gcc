// Test for GCC-894, fixed by marking functions that hold a contract assertion
// as refinable bodies: what an enforce check in a function another translation
// unit may provide implies reaches the callers compiled with it, so a
// definition linked from a TU that ignores the check turns a program without
// undefined behavior into a wrong one.
//
// In refinable-body-enforce-replaceable-e.cc the inline function f enforces
// x > 0; its violation entry point is noreturn, so f's late value-range
// pass records that f returns a positive value, and use_f, compiled after
// it there, folded `f (y) > 0' to true.
// refinable-body-enforce-replaceable-i.cc defines the same f with the check
// ignored, and the linker keeps that copy (the first it sees).  use_f (-5) must then be 0; GCC gave 1.
// f now has a refinable body (it holds a contract assertion and the
// linker may choose another definition), so use_f no longer takes f's
// return value range from it.  The P4405 proof shadows no longer rely on such
// premises (p4405-premise-*.C), but ordinary code did.  -fno-ipa-sra:
// otherwise use_f calls a local clone of its own TU's f, which hides the
// problem.
//
// Mirror: clang/test/Contracts/Runnable/enforce-ipa-replaceable.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -O2 -fno-ipa-sra" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-body-enforce-replaceable.json" }
// { dg-additional-sources "refinable-body-enforce-replaceable-i.cc refinable-body-enforce-replaceable-e.cc" }

int use_f (int);

int main ()
{
  if (use_f (-5) != 0)
    __builtin_abort ();
}
