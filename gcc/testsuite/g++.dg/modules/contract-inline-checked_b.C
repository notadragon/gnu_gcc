// Consumer for contract-inline-checked_a.C: both contracts are checked, the
// inline one from the body streamed through the CMI.

// { dg-module-do run { target c++26 } }
// { dg-additional-options "-fmodules -fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

import contract_inline_checked;
#include <contracts>

int violations;
void handle_contract_violation (const std::contracts::contract_violation &)
{ ++violations; }

int main ()
{
  ni (0);
  nn (0);
  if (violations != 2)
    __builtin_abort ();
}
