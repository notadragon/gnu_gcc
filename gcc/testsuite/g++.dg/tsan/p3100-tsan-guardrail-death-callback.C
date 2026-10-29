// Under contract routing the runtime guardrail refuses
// __sanitizer_set_death_callback, whichever sanitizer's checks are routed: a
// callback would interpose on the terminate path that contract enforcement
// drives.  Here TSan checks are routed to the contract-violation handler, and
// the setter dies with the guardrail message.
//
// Mirror: clang/test/Contracts/Runnable/p3100-tsan-guardrail-death-callback.cpp
// in the llvm_llvm-project fork.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-semantic=thread:noexcept_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "guardrail dies when routing is active" }

#include <cstdio>

extern "C" void __sanitizer_set_death_callback (void (*) (void));

static void cb () {}

int main ()
{
  // Routing is active, so this call must not return: it reports and Die()s.
  __sanitizer_set_death_callback (cb);
  std::printf ("callback installed\n");
  return 0;
}

// { dg-output "stock death callbacks are disabled under contract routing" }
