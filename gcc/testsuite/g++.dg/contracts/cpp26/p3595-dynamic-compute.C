// P3595: a compute_semantic facet transforms the dynamically selected semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-compute.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using std::contracts::evaluation_semantic;

// compute_semantic maps any selected semantic to observe.
struct to_observe_t {
  using assertion_control_object = to_observe_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const
  { return evaluation_semantic::observe; }
};
constexpr to_observe_t to_observe{};

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// Selector returns enforce; compute_semantic maps it to observe.
evaluation_semantic p3595_sel_compute() { return evaluation_semantic::enforce; }

void f(int x) pre<to_observe>(x > 0) { }

int main() {
  f(-1);                       // enforce -> compute_semantic -> observe -> handler+continue
  if (violations != 1) __builtin_abort();
}
