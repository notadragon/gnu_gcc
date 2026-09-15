// D4301: contract_violation::report() must NOT be available without
// -fcontracts-p4301, even though contracts (and the rest of the
// contract_violation API) are otherwise enabled.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

void handle_contract_violation(const std::contracts::contract_violation& v)
{
  v.report(); // { dg-error "has no member named 'report'" "" { target *-*-* } }
}
