// P3100 Task 2.2 / Task 4.1: under -fcontracts-p3100, an ASan-detected error is
// routed to the contract-violation handler.  With -fcontracts-p4298 and the
// default -fsanitize=address (no -fsanitize-recover=), the address check
// resolves to noexcept_enforce: the handler runs (reporting the violation as an
// implicit contract assertion) and the program TERMINATES after it.  The
// noexcept_enforce semantic requires -fcontracts-p4298 -- without it the
// default resolves to quick_enforce, which terminates WITHOUT the handler (see
// p3100-asan-route-quick-enforce.C).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce terminates after routing the ASan report" }

#include <contracts>
#include <cstdio>
#include <cstdlib>

// Non-throwing handler.  A throwing handler here would std::terminate() from
// inside libasan's noexcept ScopedInErrorReport destructor (Task 4.1), which is
// exactly why the routed semantic is noexcept_enforce.  Print the kind AND the
// semantic so the test can assert kind() == implicit (7) and, crucially,
// semantic() == noexcept_enforce (7) -- NOT the plain throwing enforce (3).
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
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
  // Out-of-bounds heap read: ASan detects and, under routing, reports through
  // the contract handler, then terminates (enforce).
  sink = oob (p, 100);
  std::free (p);
  // noexcept_enforce = report + terminate: we must NOT reach here.
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// The handler ran, reporting an implicit contract assertion (kind 7) with the
// NOEXCEPT_ENFORCE evaluation semantic (7), not the plain throwing enforce (3).
// RF5: the sanitizer emits NOTHING on the routed path (this handler does not
// call report()), so the handler line is the FIRST output -- anchor at ^ so any
// leaked pre-handler sanitizer text (e.g. the "=====" banner or an "ABORTING"
// line) would fail the match.
// { dg-output "^handler kind=7 semantic=7" }
// noexcept_enforce terminates (dg-shouldfail): the program dies after the handler, so
// "survived" is never printed.  The non-zero exit is asserted by dg-shouldfail
// above; a positive dg-output check for the absence of later output is not
// reliably expressible, so termination is verified via the exit status.
