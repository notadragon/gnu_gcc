// D4451R0 "Caller-Side Evaluations" (DECISIONS.md E180, GCC-966): the
// paper's reproducer.  f's second precondition dereferences p, and is
// meaningful only once the first, terminating, one has checked that p is
// not null.  A P3595 configuration asking for the caller-side check of the
// second precondition alone (p4451-callerside-skip.json) must also make
// the first one at the call, before it; GCC used to check only the second
// there, and crashed.  p4451-callerside-skip-good.C asks for both
// (p4451-callerside-skip-good.json).  Either way the caller checks
// not_null first, enforced, and the violation handler throws.  Under
// -fcontracts-p4451 alone; the -p3400 and -both files include this one.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4451-callerside-skip.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

static char trace[8];

bool
not_null (int *p)
{
  std::strcat (trace, "n");
  return p != nullptr;
}

bool
positive (int *p)
{
  std::strcat (trace, "p");
  return *p > 0;
}

struct violation {};

void
handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.semantic () != std::contracts::evaluation_semantic::enforce)
    std::abort ();
  std::strcat (trace, "v");
  throw violation {};
}

void
f (int *p)
  pre<terminating> (not_null (p))	// line 49
  pre (positive (p))			// line 50
{
}

int
main ()
{
  try
    {
      f (nullptr);
      std::abort ();
    }
  catch (violation)
    {
    }
  if (std::strcmp (trace, "nv") != 0)
    std::abort ();
}
