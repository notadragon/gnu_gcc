// P3097, DECISIONS.md E180 (5): the checks a virtual call makes of the
// contract assertions of the function it names (the interface wrapper)
// never add ignore to their allowed sets: they use the callee-side
// semantics, so with ignore configured everywhere -- also for caller-side
// checks (p4451-callerside-virtual.json) -- B::f's terminating
// precondition #1 and postcondition #3 are still enforced around D::f,
// and only the ordinary #2 is skipped.  Sequence: 1, D::f's body, 3.
// Under -fcontracts-p4451 alone; the -p3400 and -both files include this
// one.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451 -fcontracts-p3097 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4451-callerside-virtual.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

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
  if (v.semantic () != std::contracts::evaluation_semantic::enforce)
    std::abort ();
  std::strcat (trace, "v");
  throw violation {};
}

struct B
{
  virtual void f ()
    pre<terminating> (check (1))
    pre (check (2))
    post<terminating> (check (3));
};

void B::f () { }

struct D : B
{
  void f () override { std::strcat (trace, "|"); }
};

static void
run (B &b, int n, const char *seq)
{
  trace[0] = '\0';
  evaluations = 0;
  target = n;
  try { b.f (); } catch (violation) { }
  if (std::strcmp (trace, seq) != 0)
    std::abort ();
}

int
main ()
{
  D d;
  run (d, 0, "1|3");
  run (d, 1, "1v");
  run (d, 2, "1|3v");
}
