// Consumer for contract-gmf-member-spec_a.C.  Redefining the imported
// member template specialization through the header is accepted
// (check_explicit_specialization keeps the imported definition), and the
// precondition the definition carries is still checked.

// { dg-module-do run { target c++26 } }
// { dg-additional-options "-fmodules -fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

import contract_gmf_member_spec;
#include "contract-gmf-member-spec.h"
#include <contracts>

int violations;
void handle_contract_violation (const std::contracts::contract_violation &)
{ ++violations; }

int main ()
{
  N::A<int> a;
  if (a.f<long> (1) != 1 || violations != 0)
    __builtin_abort ();
  a.f<long> (0);
  if (violations != 1)
    __builtin_abort ();
}
