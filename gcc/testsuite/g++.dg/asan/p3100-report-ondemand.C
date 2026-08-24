// P3100 RF5: on the contract-routed ASan path the sanitizer emits NOTHING
// itself.  Instead it registers a lazy report populator (CXA_FIELD_REPORT) on
// the violation, and the handler renders the full ASan report ON DEMAND by
// calling contract_violation::report().  This test proves:
//   (a) nothing (no "=====" banner, no ASan text) appears BEFORE the handler,
//   (b) the full ASan report (heap-buffer-overflow, a "#0 " frame) appears only
//       BETWEEN the handler's REPORT-BEGIN and REPORT-END markers.
// With -fcontracts-p4298 and the default -fsanitize=address the routed check
// resolves to noexcept_enforce, so the program terminates after the handler
// (dg-shouldfail).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fcontracts-p4301 -fsanitize=address" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce terminates after the handler" }

#include <contracts>
#include <cstdio>
#include <cstdlib>

// Non-throwing handler.  Brackets the on-demand report with REPORT-BEGIN /
// REPORT-END so the dg-output assertion can prove the ASan text is handler-
// owned (between the markers) and that nothing leaked before REPORT-BEGIN.
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("REPORT-BEGIN\n");
  std::fflush (stdout);
  const char *r = v.report ();
  std::printf ("%s\n", r ? r : "(null)");
  std::fflush (stdout);
  std::printf ("REPORT-END\n");
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
  // Out-of-bounds heap read: ASan detects it and routes to the handler, which
  // pulls the rendered report via report().
  sink = oob (p, 100);
  std::free (p);
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// PROVABLY non-vacuous ordering assertion.  DejaGnu matches all dg-output
// patterns as one Tcl regexp against the whole output ("." crosses newlines).
// We anchor at the start (^) and require the FIRST output to be REPORT-BEGIN,
// forbidding any character before it.  Then the ASan report text
// (heap-buffer-overflow and a "#0 " symbolized frame) must appear, then
// REPORT-END.  Because of the leading ^, any pre-handler sanitizer output (the
// "=====" banner, an early "AddressSanitizer" line, etc.) shifts REPORT-BEGIN
// off the start and FAILS the match -- proving "nothing before the handler".
// (Confirmed non-vacuous: injecting any pre-handler line, or removing the
// report() call so the ASan text vanishes, breaks this assertion.)
// { dg-output "^REPORT-BEGIN\[\r\n\]+(?:.*)heap-buffer-overflow(?:.*)#0 (?:.*)REPORT-END" }
