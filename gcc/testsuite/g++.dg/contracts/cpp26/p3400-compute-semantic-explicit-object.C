// P3400: a compute_semantic facet declared with an explicit object parameter
// is called like any other: the facet is invoked with an lvalue of
// evaluation_semantic and overload resolution handles the object parameter.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

struct xobj_t {
  using assertion_control_object = xobj_t;
  constexpr evaluation_semantic
  compute_semantic (this const xobj_t&, evaluation_semantic s)
  { return s == evaluation_semantic::enforce ? evaluation_semantic::observe : s; }
};
constexpr xobj_t xobj{};
static_assert (std::contracts::labels::semantic_computation_label<xobj_t>);
static_assert (xobj.compute_semantic (evaluation_semantic::enforce)
	       == evaluation_semantic::observe);

static int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{ ++violations; }

void f (int x) pre<xobj> (x > 0) {}

int main () {
  f (-1);                           // enforce -> observe: continues
  if (violations != 1)
    __builtin_abort ();
}
