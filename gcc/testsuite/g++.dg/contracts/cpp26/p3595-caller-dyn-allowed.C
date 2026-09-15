// P3595: caller-side dynamic selection with an allowed_semantics label --
// a returned semantic outside the caller's allowed set (label | IGNORE) is
// clamped (fallback_order) before the caller-side check runs.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-dyn-allowed.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// allowed_semantics = { ignore, observe }.  "ignore" is included so the
// callee's own embedded check (same physical contract as the wrapper's
// copy) stays inert under the CLI default -- ensure_evaluation_semantic
// clamps unconditionally (not just for dynamic entries), so if "ignore"
// were excluded here the callee side would ALSO be bumped to "observe"
// and fire a second (unwanted) handler call, defeating the caller-side
// isolation this test wants.  See p3595-caller-dyn-compute-static.C for
// the analogous "compute_semantic must be identity on ignore" gotcha.
struct only_observe_t {
  using assertion_control_object = only_observe_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::ignore, evaluation_semantic::observe};
};
constexpr only_observe_t only_observe{};

static int fired = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++fired; }

// Selector returns enforce, which is NOT in the caller's allowed set
// ({ignore, observe}) -> clamp (fallback_order) reduces it to observe.
evaluation_semantic p3595_caller_sel_allowed() { return evaluation_semantic::enforce; }

int f(int x) pre<only_observe>(x > 0) { return x; }   // callee-side ignored (CLI default)
int call() { return f(-1); }                          // caller-side dynamic wrapper

int main() {
  call();
  if (fired != 1) __builtin_abort();   // enforce clamped to observe caller-side
}
