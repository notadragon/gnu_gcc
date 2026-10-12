// Consumer for contract-template-export_a.C: the imported templates'
// contracts are checked in their instantiations here.

// { dg-module-do run { target c++26 } }
// { dg-additional-options "-fmodules -fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

import contract_template_export;
#include <contracts>

int violations;
void handle_contract_violation (const std::contracts::contract_violation &)
{ ++violations; }

int main ()
{
  g1 (1);
  TS<int>{}.k (1);
  g2 (1);
  nn (1);
  if (violations != 0)
    __builtin_abort ();
  g1 (0);
  TS<int>{}.k (0);
  g2 (-1);
  nn (0);
  if (violations != 4)
    __builtin_abort ();
}
