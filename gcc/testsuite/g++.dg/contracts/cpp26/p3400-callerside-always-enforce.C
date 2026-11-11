// P3400, D4451R0 "Caller-Side Evaluations" (DECISIONS.md E180, GCC-966):
// a precondition whose label's allowed semantics exclude ignore
// (labels::always_enforce, or a label allowing only observe and enforce) is
// checked at the call whenever a later precondition is.  The configuration
// (p3400-callerside-always-enforce.json) asks for the caller-side check of
// the second precondition alone; the callee's checks are ignore, which the
// first one's label makes enforce (f) or observe (g).  Sequences: 1 2 at
// the call, then 1 in f; 3 4 at the call, then 3 in g.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3400-callerside-always-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

using std::contracts::evaluation_semantic;

struct observe_or_enforce_t
{
  using assertion_control_object = observe_or_enforce_t;
  static constexpr std::contracts::evaluation_semantic_set allowed_semantics
    = { evaluation_semantic::observe, evaluation_semantic::enforce };
};
constexpr observe_or_enforce_t observe_or_enforce {};

static char trace[16];

static bool
check (int id)
{
  std::size_t n = std::strlen (trace);
  trace[n] = '0' + id;
  trace[n + 1] = '\0';
  return true;
}

void f ()
  pre<std::contracts::labels::always_enforce> (check (1))
  pre (check (2))		// line 41
{
}

void g ()
  pre<observe_or_enforce> (check (3))
  pre (check (4))		// line 47
{
}

int
main ()
{
  f ();
  if (std::strcmp (trace, "121") != 0)
    std::abort ();
  trace[0] = '\0';
  g ();
  if (std::strcmp (trace, "343") != 0)
    std::abort ();
}
