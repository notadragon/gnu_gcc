// OPEN BUG (GCC-806 in gnu_gcc open-issues/; deferred, DECISIONS.md E90: a
// documented limitation).
//
// The effects of the contract-violation handler reached from the routed
// address check, which the middle end inserts after the optimizers that move
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
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-semantic=address:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <cstdlib>

static int reports;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++reports;
  std::puts ("H");
}

static volatile int vz = 0;

int
main ()
{
  int *h = (int *) std::malloc (4 * sizeof (int));
  for (int k = 0; k < 4; ++k)
    h[k] = k + vz;
  int i = vz + 4;
  int before = reports;
  volatile int x = h[i];
  std::printf ("address %d\n", reports - before);
  (void) x;
  std::free (h);
  return 0;
}

// { dg-output "H.*address 1" { xfail *-*-* } }
