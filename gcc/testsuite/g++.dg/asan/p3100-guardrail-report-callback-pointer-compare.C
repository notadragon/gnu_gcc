// P3100: runtime guardrail with only pointer-compare routed.  With the
// address check at assume (no descriptor) and pointer-compare routed,
// __asan_set_error_report_callback must still refuse a stock callback, since
// pointer-compare reports go to the contract-violation handler.
// p3100-guardrail-report-callback.C is the address-routed case.
//
// Mirror: clang/test/Contracts/Runnable/p3100-asan-guardrail-report-callback-pointer-compare.cpp
// in the llvm_llvm-project fork.
// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fcontracts-allow-assume -fsanitize=pointer-compare -fsanitize-semantic=address:assume,pointer-compare:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "the guardrail aborts when any check is routed" }

#include <cstdio>

extern "C" void __asan_set_error_report_callback (void (*) (const char *));

static void my_cb (const char *) {}

int main ()
{
  __asan_set_error_report_callback (my_cb);
  std::printf ("registered\n");
  return 0;
}

// { dg-output "stock error-report callbacks are disabled under contract routing" }
