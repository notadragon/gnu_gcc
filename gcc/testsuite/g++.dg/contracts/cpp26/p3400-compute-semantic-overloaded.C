// P3400: a public compute_semantic facet overloaded with a private
// tag-dispatch helper of the same name (the pattern P3400 cites as
// legitimate) is applied, whichever of the two is declared first: overload
// resolution picks the facet, and access is judged on the member chosen.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

struct tag {};
struct A {
  using assertion_control_object = A;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic s) const
  { return compute_semantic (tag{}, s); }
private:
  constexpr evaluation_semantic compute_semantic (tag, evaluation_semantic) const
  { return evaluation_semantic::observe; }
};
struct B {
  using assertion_control_object = B;
private:
  constexpr evaluation_semantic compute_semantic (tag, evaluation_semantic) const
  { return evaluation_semantic::observe; }
public:
  constexpr evaluation_semantic compute_semantic (evaluation_semantic s) const
  { return compute_semantic (tag{}, s); }
};
constexpr A a{};
constexpr B b{};
static_assert (std::contracts::labels::semantic_computation_label<A>);
static_assert (std::contracts::labels::semantic_computation_label<B>);

static int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{ ++violations; }

void f (int x) pre<a> (x > 0) {}
void g (int x) pre<b> (x > 0) {}

int main () {
  f (-1);                           // enforce -> observe: continues
  g (-1);
  if (violations != 2)
    __builtin_abort ();
}
