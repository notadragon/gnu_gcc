// P3100 Task 3.1 + 3.2: the -fsanitize-noncontract-callbacks opt-out disengages
// BOTH the report routing and the runtime guardrail.  With the opt-out, no
// routing descriptor is emitted, so __asan_set_error_report_callback registers
// the stock callback normally (no abort at the call) and stock ASan reporting
// runs through it.  This is the same program as p3100-guardrail-report-callback.C
// but built with the opt-out; there the call aborts, here it succeeds.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fsanitize-recover=address -fsanitize-noncontract-callbacks" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// With the opt-out there is no contract routing: stock ASan handles the
// out-of-bounds access and, by default, aborts after reporting.  The point of
// this test is that the callback REGISTERS (guardrail disengaged) and stock
// reporting runs through it; the abort is stock behavior, hence dg-shouldfail.
// { dg-shouldfail "stock ASan aborts after its report" }

#include <cstdio>
#include <cstdlib>

extern "C" void __asan_set_error_report_callback (void (*) (const char *));

static void my_cb (const char *)
{
  // Stock callback path: reached when ASan produces its report.
  std::printf ("stock callback ran\n");
  std::fflush (stdout);
}

volatile int sink;

int __attribute__((noinline))
oob (int *p, int i)
{
  return p[i];
}

int main ()
{
  // With the opt-out, the guardrail is disengaged: this registers normally.
  __asan_set_error_report_callback (my_cb);
  std::printf ("registered\n");
  std::fflush (stdout);

  int *p = (int *) std::malloc (4 * sizeof (int));
  sink = oob (p, 100);
  std::free (p);
  return 0;
}

// The callback registered without aborting (guardrail disengaged).
// { dg-output "registered" }
// Stock ASan reporting still runs (no contract routing) ...
// { dg-output ".*ERROR: AddressSanitizer: heap-buffer-overflow" }
// ... and drives the stock callback we installed.
// { dg-output ".*stock callback ran" }
