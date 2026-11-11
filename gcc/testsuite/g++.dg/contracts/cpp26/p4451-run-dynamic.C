// P4451: a semantic selected at run time (P3595 dynamic selection) is
// adjusted for a terminating contract assertion as for P3400's
// labels::terminating: a selected ignore or observe is evaluated as
// enforce.  Under -fcontracts-p4451 alone.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4451-dynamic.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static evaluation_semantic selected;

evaluation_semantic
p4451_sel ()
{
  return selected;
}

static int handled;

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.semantic () != evaluation_semantic::enforce)
    std::abort ();
  ++handled;
  throw handled;
}

int f (int x) pre<terminating> (x > 0) { return x; }
void g (int x) { contract_assert<terminating> (x > 0); }

int
main ()
{
  for (evaluation_semantic s : { evaluation_semantic::ignore,
				 evaluation_semantic::observe,
				 evaluation_semantic::enforce })
    {
      selected = s;
      try { f (-1); std::abort (); } catch (int) { }
      try { g (-1); std::abort (); } catch (int) { }
    }
  if (handled != 6)
    std::abort ();
}
