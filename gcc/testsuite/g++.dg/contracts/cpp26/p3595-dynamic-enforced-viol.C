// P3595: a compute_semantic result that is not in the allowed_semantics set
// for the DYNAMICALLY selected semantic produces an unconditional enforced
// violation at runtime (the sentinel path), even though the predicate is
// true.  The compile-time default semantic ("observe" from the config) must
// still map to an allowed value, or this would instead be a compile error.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-enforced-viol.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// allowed_semantics has two elements so the clamp preserves the distinction
// between the config default (observe) and the selector's return (enforce).
// compute_semantic then maps enforce to quick_enforce, which is NOT allowed.
struct enforce_to_quick_t {
  using assertion_control_object = enforce_to_quick_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe, evaluation_semantic::enforce};
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic s) const
  {
    return s == evaluation_semantic::enforce
      ? evaluation_semantic::quick_enforce
      : s;
  }
};
constexpr enforce_to_quick_t enforce_to_quick{};

// Selector returns enforce; combined with compute_semantic above, the
// dynamic path is forced to the disallowed quick_enforce value.
evaluation_semantic p3595_sel_ev() { return evaluation_semantic::enforce; }

// Reaching the handler proves the unconditional enforced violation fired.
void handle_contract_violation(const std::contracts::contract_violation&) {
  std::exit(0);
}

void f(const int x) pre<enforce_to_quick>(x > 0) { }

int main() {
  f(1);                         // predicate is TRUE, but the dynamic result
                                 // maps to a disallowed semantic, so the
                                 // enforced violation still fires unconditionally
  __builtin_abort ();            // unreachable: handler above should exit(0)
}
