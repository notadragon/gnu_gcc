// P3100 Bug #3 (routed behavior depends ONLY on the configured semantic, not on
// ASAN_OPTIONS).  Under the default harness env (halt_on_error and
// suppress_equal_pcs both at their defaults) the pre-fix runtime mishandled
// routed noexcept_observe two ways:
//   * the SECOND violation at a DIFFERENT site was dropped -- the first report
//     consumed the process-wide crash-state latch, so the second hit the
//     `halt_on_error_ && !__sanitizer_acquire_crash_state()` gate and
//     early-returned without invoking the handler;
//   * whether a same-site repeat re-fired depended on suppress_equal_pcs.
// After the fix the routed path ignores those env-driven gates: every distinct
// site is delivered, and a same-site repeat is reported ONCE (deterministically,
// matching "continue as if the check were not present" for repeats).  So the
// handler is called exactly twice here -- once per distinct site -- regardless
// of ASAN_OPTIONS, and the program continues to completion.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-semantic=address:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <cstdlib>

static int calls = 0;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++calls;
  std::printf ("handler %d\n", calls);
  std::fflush (stdout);
}

volatile int sink;

int __attribute__((noinline))
oob (int *p, int i)
{
  return p[i];			// site 1's PC (also reused by the loop below)
}

int main (int argc, char **)
{
  int *p = (int *) std::malloc (3 * sizeof (int));
  int i = argc + 4;		// >= 5, heap OOB, not constant-foldable
  sink = oob (p, i);		// site 1 (oob's PC)
  sink = p[i + 1];		// site 2 (a distinct PC, inlined here)
  for (int k = 0; k < 3; ++k)	// same PC as site 1 -> reported once, not thrice
    sink = oob (p, i);
  std::free (p);
  std::printf ("done calls=%d\n", calls);
  std::fflush (stdout);
  return 0;
}

// Both distinct sites reported; the same-site repeat added no further call.
// { dg-output "done calls=2" }
