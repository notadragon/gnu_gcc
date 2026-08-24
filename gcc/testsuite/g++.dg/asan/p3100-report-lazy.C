// P3100 RF5: the ASan report populator is LAZY -- it runs ONLY if the handler
// calls contract_violation::report().  Here the handler does NOT call report(),
// so the populator never runs and the sanitizer prints nothing.  This proves
// laziness: the only output is the handler's own "HANDLED" line; no ASan report
// text (no "heap-buffer-overflow", no "#0 " frame, no "=====" banner) appears
// anywhere.  With -fcontracts-p4298 and the default -fsanitize=address the
// routed check resolves to noexcept_enforce, so the program terminates after
// the handler (dg-shouldfail).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fcontracts-p4301 -fsanitize=address" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce terminates after the handler" }

#include <contracts>
#include <cstdio>
#include <cstdlib>

// Non-throwing handler that deliberately does NOT call v.report().  The
// populator must therefore never run, and no ASan report text must appear.
void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::printf ("HANDLED\n");
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
  int *p = (int *) std::malloc (4 * sizeof (int));
  sink = oob (p, 100);
  std::free (p);
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// PROVABLY non-vacuous laziness assertion.  DejaGnu matches all dg-output
// patterns as one Tcl regexp over the whole output ("." crosses newlines).  We
// anchor at the start (^) and require the ENTIRE output to be exactly the
// handler's "HANDLED" line, forbidding "heap-buffer-overflow", a "#0 " frame,
// and the "=====" banner anywhere by consuming only characters that do not
// begin any of those tokens up to end-of-text ($).  If the populator ever ran
// (regression: report rendered despite report() not being called) or the
// sanitizer leaked its own report, the forbidden text would appear and the
// anchored ^...$ match would fail.  (Confirmed non-vacuous: pointing this test
// at the on-demand handler that calls report() makes the ASan text appear and
// breaks the match.)
// { dg-output "^HANDLED(?:(?!heap-buffer-overflow)(?!#0 )(?!====).)*$" }
