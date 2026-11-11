// D4451R0 "Caller-Side Evaluations" (DECISIONS.md E180, E181 (1), GCC-969)
// with a P3595 dynamic selection of the caller-side check of a later
// precondition: f's terminating precondition #1 has no caller-side check
// configured; the caller-side check of its ordinary precondition #2 is
// chosen at run time by a selector (p4451-callerside-dynamic-later.json).
// A dynamically selected semantic counts as not ignored, whatever the
// selector returns, so #1 is checked at the call, as enforce, before #2:
//   selector ignore:            1 12 (#2 skipped at the call)
//   selector observe, enforce:  12 12
// and when #1's check fails at the call, #2 is not evaluated there.  g is
// the same with #1's caller-side check also chosen by the selector: #1 is
// then made at the call whatever the selector returns (M5's case of an
// earlier dynamic precondition), with the same sequences.
// Under -fcontracts-p4451 alone; the -p3400 and -both files include this
// one.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4451-callerside-dynamic-later.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

using std::contracts::evaluation_semantic;

static evaluation_semantic selected;

evaluation_semantic
p4451_callerside_later_sel ()
{
  return selected;
}

static char trace[16];
static int evaluations, target;

static bool
check (int id)
{
  std::size_t n = std::strlen (trace);
  trace[n] = '0' + id;
  trace[n + 1] = '\0';
  return ++evaluations != target;
}

struct violation {};

void
handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.semantic () != evaluation_semantic::enforce)
    std::abort ();
  std::strcat (trace, "v");
  throw violation {};
}

void f ()
  pre<terminating> (check (1))
  pre (check (2)) { }		// line 60: dynamic at the call

void g ()
  pre<terminating> (check (1))	// line 63: dynamic at the call
  pre (check (2)) { }		// line 64: dynamic at the call

static void
run (void (*fn) (), const char *seq)
{
  trace[0] = '\0';
  evaluations = 0;
  target = 0;
  fn ();
  if (std::strcmp (trace, seq) != 0)
    std::abort ();
  /* #1's check at the call fails: enforced, so the sequence stops.  */
  trace[0] = '\0';
  evaluations = 0;
  target = 1;
  try { fn (); std::abort (); } catch (violation) { }
  if (std::strcmp (trace, "1v") != 0)
    std::abort ();
}

int
main ()
{
  for (evaluation_semantic s : { evaluation_semantic::ignore,
				 evaluation_semantic::observe,
				 evaluation_semantic::enforce })
    {
      selected = s;
      const char *seq = s == evaluation_semantic::ignore ? "112" : "1212";
      run ([] { f (); }, seq);
      run ([] { g (); }, seq);
    }
}
