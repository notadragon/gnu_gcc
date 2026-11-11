// OPEN BUG (GCC-806 in gnu_gcc open-issues/; deferred, DECISIONS.md E90: a
// documented limitation).
//
// The effects of the contract-violation handler reached from the routed
// thread check, which the middle end inserts after the optimizers that move
// loads have run, are not seen by the code after the check at -O1 and
// above: `reports', a static only the handler changes, is read before the
// access and its value reused after it.  The check reports once (the
// handler prints "H") and the program continues, but the count printed is
// 0; at -O0 it is 1.  The checks the front end and pass_ubsan emit are not
// affected (GCC-805; g++.dg/ubsan/p3100-routed-handler-effects.C).  Only
// the -O2 variant runs; when it prints 1 the output test XPASSes.
// Mirror of llvm_llvm-project
// clang/test/Contracts/OpenBugs/p3100-routed-handler-effects-late.cpp
// (CLANG-761, the same limitation).
// { dg-do run }
// { dg-skip-if "" { *-*-* } { "*" } { "-O2" } }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-semantic=thread:noexcept_observe -ldl" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>

static int reports;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++reports;
  std::puts ("H");
}

int shared;
static void *
writer (void *)
{
  shared = 1;
  return nullptr;
}

int
main ()
{
  pthread_t t;
  pthread_create (&t, nullptr, writer, nullptr);
  usleep (100000);
  int before = reports;
  shared = 2;
  std::printf ("thread %d\n", reports - before);
  pthread_join (t, nullptr);
  return 0;
}

// { dg-output "H.*thread 1" { xfail *-*-* } }
