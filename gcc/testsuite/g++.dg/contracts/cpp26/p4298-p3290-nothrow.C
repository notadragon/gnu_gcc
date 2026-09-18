// D4298 + P3290: the nothrow_t overloads report noexcept_enforce /
// noexcept_observe, not enforce/observe, when P3290 is enabled.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3290 -fcontracts-p4298" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using namespace std::contracts;

static evaluation_semantic seen = evaluation_semantic::unspecified;
void handle_contract_violation(const contract_violation& v) { seen = v.semantic(); }

int main()
{
  handle_observed_contract_violation(std::nothrow, "msg");
  if (seen != evaluation_semantic::noexcept_observe) __builtin_abort ();
}
