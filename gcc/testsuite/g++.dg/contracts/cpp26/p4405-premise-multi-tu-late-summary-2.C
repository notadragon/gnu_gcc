// P4405 premises decided by this TU (T1-GCC), across translation units: nothing
// an assumption implies leaves a replaceable function through a late summary.
// In one TU the inline function f proves and assumes x > 0, which its late
// value-range pass sees in its result; use_f, compiled after it in the same TU,
// must not fold `f (y) > 0' to true, since the linker may keep another TU's f,
// which ignores its assertions.  The linker keeps the first copy it sees: the
// ignoring one, so use_f (-5) is 0; before the rule it was 1.
//
// Mirror: clang/test/Contracts/Runnable/p4405-premise-multi-tu-late-summary.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-multi-tu-late-summary.json" }
// { dg-additional-sources "p4405-premise-multi-tu-late-summary-i.cc p4405-premise-multi-tu-late-summary-e.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <csignal>
#include <cstdlib>

int use_f (int);

int main ()
{
  if (use_f (-5) != 0)
    __builtin_abort ();
}

