// P4405 premises decided by this TU (M5-GCC), across translation units: a proof
// may not rely on what a replaceable callee's code does not do unless the real
// function inlined it.  f's shadow inlines the inline function h, whose
// precondition f's TU ignores, so it sees that g is still 1 after the call, but
// f calls h out of line, and the linker keeps the other TU's h, which observes
// the precondition: h (-5) calls the violation handler, which sets g to 0 and
// returns.  f (1, -5) is h (-5) + 20 = 11; had f assumed that g is 1 after the
// call, it would return 1.
//
// Mirror: clang/test/Contracts/Runnable/p4405-premise-multi-tu-effects.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "--param large-function-insns=40 --param large-function-growth=10" }
// { dg-additional-options "--param max-inline-insns-single=200 --param max-inline-insns-auto=200 -fno-partial-inlining" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-multi-tu-effects.json" }
// { dg-additional-sources "p4405-premise-multi-tu-effects-o.cc p4405-premise-multi-tu-effects-e.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>

int g;
[[gnu::const, gnu::noinline]] int sinkc (int v) { return v; }
[[gnu::noinline]] int big (int v) { return v; }
int f (int, int);

void handle_contract_violation (const std::contracts::contract_violation &)
{
  g = 0;
}

int main ()
{
  if (f (1, -5) != 11)
    __builtin_abort ();
}

