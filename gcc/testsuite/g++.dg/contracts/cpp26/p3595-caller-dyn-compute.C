// P3595: caller-side dynamic selection with a compute_semantic label --
// the two-stage transform (clamp then compute_semantic) applies to the
// caller-side dispatch exactly as it does callee-side.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-dyn-compute.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using std::contracts::evaluation_semantic;

// compute_semantic maps enforce -> observe, identity otherwise (crucially,
// ignore -> ignore).  The callee-side default is ignore (CLI
// -fcontract-evaluation-semantic=ignore); ensure_evaluation_semantic applies
// compute_semantic unconditionally (even to the opt-out default), so a
// facet that transformed "ignore" too would incorrectly wake up a second,
// callee-side check on the same physical contract the wrapper copies from.
// See p3595-caller-dyn-compute-static.C for the callee-side analogue of
// this gotcha.
struct to_observe_t {
  using assertion_control_object = to_observe_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const
  { return s == evaluation_semantic::enforce
      ? evaluation_semantic::observe : s; }
};
constexpr to_observe_t to_observe{};

static int fired = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++fired; }

// Selector returns enforce; compute_semantic maps it to observe.
evaluation_semantic p3595_caller_sel_compute() { return evaluation_semantic::enforce; }

int f(int x) pre<to_observe>(x > 0) { return x; }   // callee-side ignored (CLI default)
int call() { return f(-1); }                        // caller-side dynamic wrapper

int main() {
  call();
  if (fired != 1) __builtin_abort();   // enforce -> compute_semantic -> observe
}
