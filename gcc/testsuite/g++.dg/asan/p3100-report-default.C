// P3100 RF6: end-to-end default-handler test tying RF4 and RF5 together.
// There is deliberately NO user-defined handle_contract_violation here, so the
// library's own default handler (contract26.cc) runs.  RF4 wired that default
// handler to call contract_violation::report() and print it before its usual
// "contract violation in function ..." summary line; RF5 made the ASan routed
// path register a lazy report populator instead of printing anything itself.
// This test proves the two halves compose end-to-end: on a routed ASan error
// with no user handler, the default handler's own call to report() renders
// the full ASan diagnostic (heap-buffer-overflow, a "#0 " frame), and that
// text is followed by the default handler's "contract violation in function"
// / "assertion_kind: implicit" summary -- proving the report came from the
// default handler (via report()), not from a stock, un-routed ASan report
// that bypassed contract handling entirely.  With -fcontracts-p4298 and the
// default -fsanitize=address, the routed check resolves to noexcept_enforce,
// so the program terminates after the default handler runs (dg-shouldfail).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fcontracts-p4301 -fsanitize=address" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce terminates after the default handler" }

#include <contracts>
#include <cstdio>
#include <cstdlib>

// No user handle_contract_violation -- the library's default handler runs.

volatile int sink;

int __attribute__((noinline))
oob (int *p, int i)
{
  return p[i];
}

int main ()
{
  int *p = (int *) std::malloc (4 * sizeof (int));
  // Out-of-bounds heap read: ASan detects it and routes to the default
  // handler, which pulls the rendered report via report() and prints it.
  sink = oob (p, 100);
  std::free (p);
  // noexcept_enforce = report + terminate: we must NOT reach here.
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// The default handler prints report() (the ASan diagnostic: heap-buffer-
// overflow and a "#0 " symbolized frame) first, then its own "contract
// violation in function" summary line naming the implicit contract assertion
// with the noexcept_enforce semantic.  Requiring the ASan text to be
// immediately followed by the default handler's own summary proves the report
// was rendered ON the routed/default-handler path, not by a stock, un-routed
// ASan abort that never reached contract handling.
// { dg-output "heap-buffer-overflow(?:.*)#0 (?:.*)contract violation in function" }
// { dg-output "(?:.*)assertion_kind: implicit(?:.*)semantic: noexcept_enforce" }
