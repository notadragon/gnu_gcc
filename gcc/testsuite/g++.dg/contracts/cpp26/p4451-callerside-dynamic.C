// D4451R0 "Caller-Side Evaluations" (DECISIONS.md E180, GCC-966) with a
// P3595 dynamic selection of the caller-side check of the terminating
// precondition #1 (p4451-callerside-dynamic.json):
//   f: #2 is checked at the call (observe), so #1 is made there whatever
//      the selector returns, as enforce: 12 12;
//   g: #2 is not checked at the call, so a selected ignore skips #1 there
//      (12), and observe or enforce make it, as enforce (1 12).
// Under -fcontracts-p4451 alone; the -p3400 and -both files include this
// one.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4451-callerside-dynamic.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

using std::contracts::evaluation_semantic;

static evaluation_semantic selected;

evaluation_semantic
p4451_callerside_sel ()
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
  pre<terminating> (check (1))	// line 53: dynamic at the call
  pre (check (2)) { }		// line 54: observe at the call

void g ()
  pre<terminating> (check (1))	// line 57: dynamic at the call
  pre (check (2)) { }

static void
run (void (*fn) (), const char *seq)
{
  trace[0] = '\0';
  evaluations = 0;
  target = 0;
  fn ();
  if (std::strcmp (trace, seq) != 0)
    std::abort ();
  /* The first check fails: enforced, so the sequence stops.  */
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
      run ([] { f (); }, "1212");
      run ([] { g (); }, s == evaluation_semantic::ignore ? "12" : "112");
    }
}
