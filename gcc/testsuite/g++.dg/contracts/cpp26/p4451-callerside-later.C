// D4451R0 "Caller-Side Evaluations" (DECISIONS.md E180, E181 (2)): when
// the caller-side evaluation of a later precondition is not ignored, an
// earlier terminating precondition is checked at the call before it.  The
// semantics whose run-time effect is that of ignore -- ignore, and P4405's
// prove and diagnose -- leave it ignored; every other one does not
// (p4451-callerside-later.json gives #2 of each function the semantic in
// its name at the call).  Sequences made, caller side then callee side:
//   prove, diagnose: #1 is not made at the call, and #2 has no run-time
//     effect there: 1
//   assume: #1 is made at the call (#2 is assumed, not evaluated): 1 1
//   enforce, quick_enforce, noexcept_observe, noexcept_enforce: 12 12
// Under -fcontracts-p4451 alone; the -p3400 and -both files include this
// one.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451 -fcontracts-p4405 -fcontracts-p4298 -fcontracts-allow-assume" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4451-callerside-later.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

static char trace[32];

static bool
check (int id)
{
  std::size_t n = std::strlen (trace);
  trace[n] = '0' + id;
  trace[n + 1] = '\0';
  return true;
}

/* The arguments are constants, so prove and diagnose prove #2 at the call
   and give no diagnostic.  */

void prove (int x)
  pre<terminating> (check (1))
  pre (x > 0) { }				// line 39

void diagnose (int x)
  pre<terminating> (check (1))
  pre (x > 0) { }				// line 43

void assume (int x)
  pre<terminating> (check (1))
  pre (x > 0) { }				// line 47

void enforce ()
  pre<terminating> (check (1))
  pre (check (2)) { }				// line 51

void quick_enforce ()
  pre<terminating> (check (1))
  pre (check (2)) { }				// line 55

void noexcept_observe ()
  pre<terminating> (check (1))
  pre (check (2)) { }				// line 59

void noexcept_enforce ()
  pre<terminating> (check (1))
  pre (check (2)) { }				// line 63

static void
run (void (*g) (), const char *seq)
{
  trace[0] = '\0';
  g ();
  if (std::strcmp (trace, seq) != 0)
    std::abort ();
}

int
main ()
{
  run ([] { prove (1); }, "1");
  run ([] { diagnose (1); }, "1");
  run ([] { assume (1); }, "11");
  run ([] { enforce (); }, "1212");
  run ([] { quick_enforce (); }, "1212");
  run ([] { noexcept_observe (); }, "1212");
  run ([] { noexcept_enforce (); }, "1212");
}
