// P4405 premises decided by this TU (T1-GCC), across translation units: nothing
// an assumption implies leaves a replaceable function through a late summary.
// In one TU the inline function f proves and assumes x > 0, which its late
// value-range pass sees in its result; use_f, compiled after it in the same TU,
// must not fold `f (y) > 0' to true, since the linker may keep another TU's f,
// which ignores its assertions.  The linker keeps the first copy it sees: the
// asserting one, so use_f (-5) traps (quick_enforce), which the handler ends
// successfully.
//
// Mirror: clang/test/Contracts/Runnable/p4405-premise-multi-tu-late-summary.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-multi-tu-late-summary.json" }
// { dg-require-effective-target signal }
// { dg-additional-sources "p4405-premise-multi-tu-late-summary-e.cc p4405-premise-multi-tu-late-summary-i.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <csignal>
#include <cstdlib>

int use_f (int);

extern "C" void on_trap (int) { std::_Exit (0); }

int main ()
{
  std::signal (SIGILL, on_trap);
  std::signal (SIGTRAP, on_trap);
  use_f (-5);
  __builtin_abort ();
}

