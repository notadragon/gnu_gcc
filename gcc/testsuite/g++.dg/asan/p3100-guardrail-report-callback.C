// P3100 Task 3.2: runtime guardrail.  When contract routing is active (the
// front end emitted the descriptor under -fcontracts-p3100 with the address
// check resolved to a routed semantic), calling the stock setter
// __asan_set_error_report_callback must abort with a fatal error naming the
// -fsanitize-noncontract-callbacks opt-out, rather than silently registering a
// stock callback that mixes with contract routing.
//
// P3100 Task 4.1: -fsanitize-recover=address routes to noexcept_observe, which
// requires -fcontracts-p4298; the guardrail fires the same way regardless of
// which routed semantic (quick_enforce / noexcept_observe / noexcept_enforce)
// is in effect.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-recover=address" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "guardrail aborts when routing is active" }

#include <cstdio>

extern "C" void __asan_set_error_report_callback (void (*) (const char *));

static void my_cb (const char *) {}

int main ()
{
  // Routing is active, so this call must not return: it reports and Die()s.
  __asan_set_error_report_callback (my_cb);
  std::printf ("registered\n");
  return 0;
}

// The guardrail fires with a message naming the opt-out.
// { dg-output "stock error-report callbacks are disabled under contract routing" }
// { dg-output ".*-fsanitize-noncontract-callbacks" }
