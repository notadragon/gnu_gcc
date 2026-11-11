// P3595: caller-side dynamic selection where a compute_semantic result
// falls outside the caller's allowed set (label | IGNORE) -- the sentinel
// path fires an unconditional enforced violation at the call site, even
// though the predicate is true.  The compile-time default ("observe" from
// the config) must still map to an allowed value, or this would instead be
// a compile error.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-dyn-enforced-viol.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// allowed_semantics includes ignore/observe/enforce: "ignore" keeps the
// callee's own embedded check (same physical contract the wrapper copies)
// genuinely inert under the CLI default (ensure_evaluation_semantic clamps
// unconditionally, even for the opt-out default -- see
// p3595-caller-dyn-allowed.C); observe+enforce preserve the distinction
// between the config default (observe) and the selector's return (enforce).
// compute_semantic then maps enforce to quick_enforce, which is NOT allowed.
struct enforce_to_quick_t {
  using assertion_control_object = enforce_to_quick_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::ignore, evaluation_semantic::observe,
     evaluation_semantic::enforce};
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
// caller-side dynamic path is forced to the disallowed quick_enforce value.
evaluation_semantic p3595_caller_sel_enforced_viol()
{ return evaluation_semantic::enforce; }

// Reaching the handler proves the unconditional enforced violation fired.
void handle_contract_violation(const std::contracts::contract_violation&) {
  std::exit(0);
}

int f(const int x) pre<enforce_to_quick>(x > 0) { return x; }  // callee-side ignored
int call() { return f(1); }                                    // caller-side wrapper

int main() {
  call();                       // predicate is TRUE, but the dynamic result
                                 // maps to a disallowed semantic caller-side,
                                 // so the enforced violation still fires
  __builtin_abort ();            // unreachable: handler above should exit(0)
}
