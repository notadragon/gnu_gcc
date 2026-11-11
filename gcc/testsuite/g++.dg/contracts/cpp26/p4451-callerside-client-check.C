// D4451R0 "Caller-Side Evaluations" (DECISIONS.md E180) with the
// caller-side checks of -fcontracts-client-check=pre, which checks every
// precondition at the call: the terminating #1 and the ordinary #2 are
// made at the call, then in f, as before E180.  Sequence: 12 12.  Under
// -fcontracts-p4451 alone; the -p3400 and -both files include this one.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451 -fcontracts-client-check=pre" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

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
  pre<terminating> (check (1))
  pre (check (2)) { }

int
main ()
{
  f ();
  if (std::strcmp (trace, "1212") != 0)
    std::abort ();
}
