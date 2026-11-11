// P4451: a configured quick_enforce is a terminating semantic and is kept
// for a terminating contract assertion: the violation traps without calling
// the handler, which would exit successfully.  Under -fcontracts-p3400 and -fcontracts-p4451 (the P3400 label).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p4451 -fcontract-evaluation-semantic=quick_enforce" }
// { dg-shouldfail "quick_enforce traps" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

int f (int x) pre<terminating> (x > 0) { return x; }

int
main ()
{
  return f (-1);
}
