// Consumer for contract-p4451-stream-both_a.C.
// { dg-module-do run { target c++26 } }
// { dg-additional-options "-fmodules -fcontracts-p3400 -fcontracts-p4451 -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

import p4451s_both;
#include <contracts>
#include <cstdlib>

static int handled;

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.semantic () != std::contracts::evaluation_semantic::enforce)
    std::abort ();
  ++handled;
  throw handled;
}

int main ()
{
  try { fi (-1); std::abort (); } catch (int) { }
  try { fn (-1); std::abort (); } catch (int) { }
  try { ft (-1); std::abort (); } catch (int) { }
  if (handled != 3)
    std::abort ();
}
