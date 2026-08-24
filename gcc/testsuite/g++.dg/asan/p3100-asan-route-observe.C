// P3100 Task 2.2 / Task 4.1: under -fcontracts-p3100, an ASan-detected error is
// routed to the contract-violation handler.  With -fcontracts-p4298 and
// -fsanitize-recover=address, the address check resolves to noexcept_observe:
// the handler runs (reporting the violation as an implicit contract assertion)
// and the program CONTINUES.  The noexcept_observe semantic requires
// -fcontracts-p4298 -- -fsanitize-recover=address without it is a hard error
// (see p3100-asan-route-recover-no-p4298.C), since there is no non-throwing way
// to honor continue-on-violation otherwise.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-recover=address" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// The routing descriptor is emitted in the per-TU front-end compile as a
// preserved weak global, so it streams through LTO and routing works under
// -flto exactly as it does non-LTO -- no LTO variants are skipped.

#include <contracts>
#include <cstdio>
#include <cstdlib>

// Non-throwing handler.  A throwing handler here would std::terminate() from
// inside libasan's noexcept ScopedInErrorReport destructor (Task 4.1), which is
// exactly why the routed semantic is noexcept_observe.  Print the kind AND the
// semantic so the test can assert kind() == implicit (7) and, crucially,
// semantic() == noexcept_observe (6) -- NOT the plain throwing observe (2).
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
  // the contract handler.
  sink = oob (p, 100);
  std::free (p);
  // noexcept_observe = report + continue: we reach here after the violation.
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// The handler ran, reporting an implicit contract assertion (kind 7) with the
// NOEXCEPT_OBSERVE evaluation semantic (6), not the plain throwing observe (2).
// RF5: the sanitizer emits NOTHING on the routed path (this handler does not
// call report()), so the handler line is the FIRST output -- anchor at ^ so any
// leaked pre-handler sanitizer text (e.g. the "=====" banner) would fail the
// match.
// { dg-output "^handler kind=7 semantic=6" }
// noexcept_observe continues: "survived" is printed after the detected access.
// { dg-output ".*survived" }
