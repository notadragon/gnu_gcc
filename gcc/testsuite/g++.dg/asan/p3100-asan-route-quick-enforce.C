// P3100 Task 4.1 / RF5: with -fcontracts-p3100 but WITHOUT -fcontracts-p4298,
// the default -fsanitize=address (no -fsanitize-recover=) resolves the routed
// address check to quick_enforce: the program TERMINATES and the contract-
// violation handler is NEVER called (the "quick" path does not enter the
// handler at all).  As of RF5 the sanitizer emits NOTHING on the routed path,
// so quick_enforce is now a SILENT fast terminate: no ASan report, no ABORTING
// line, and no handler output at all.  This is the new default behavior for a
// plain -fcontracts-p3100 -fsanitize=address build.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "quick_enforce terminates silently" }

#include <contracts>
#include <cstdio>
#include <cstdlib>

// This handler must NOT run under quick_enforce: quick_enforce terminates
// without ever entering the contract-violation handler.  If it were called it
// would print "handler ..." -- the dg-output check below asserts it is not.
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d\n", (int) v.kind ());
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
  // Out-of-bounds heap read: ASan detects and, under quick_enforce, terminates
  // silently without calling the handler and without printing anything.
  sink = oob (p, 100);
  std::free (p);
  // quick_enforce = terminate: we must NOT reach here.
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// RF5: quick_enforce is a SILENT fast terminate.  The sanitizer prints nothing
// on the routed path and the handler is never called, so the program produces
// NO output at all before dying (dg-shouldfail asserts the non-zero exit).
// This must be a PROVABLY non-vacuous assertion: DejaGnu matches all dg-output
// patterns as one Tcl regexp against the whole output (where "." crosses
// newlines).  We anchor at the start (^) and require the ENTIRE output to be
// only optional whitespace up to end-of-text ($).  Anchoring both ends removes
// any "restart/backtrack later" escape hatch, so ANY emitted character -- a
// stray "handler ..." line (regression: handler wrongly called), an
// "AddressSanitizer" report line, or the "ABORTING" banner (regression:
// sanitizer output not suppressed) -- makes the match fail.  Verified with
// tclsh: "^\\s*$" (Tcl -all -inline) matches empty output but fails the moment
// any non-whitespace text is injected anywhere.
// { dg-output "^\[ \t\r\n\]*$" }
