// P3595 x P3400: callee-side identity-on-ignore.  A compute_semantic label that
// maps enforce->observe but is the identity on every other value must leave a
// callee-side base semantic of "ignore" unchanged: compute_semantic(ignore) ==
// ignore, so no check is emitted and the handler never fires.  (The caller-side
// counterpart is p3595-caller-dyn-compute-static.C; this is the dedicated
// callee-side case.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
using std::contracts::evaluation_semantic;

struct enforce_to_observe_t {
  using assertion_control_object = enforce_to_observe_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const
  { return s == evaluation_semantic::enforce ? evaluation_semantic::observe : s; }
};
constexpr enforce_to_observe_t e2o{};

static int fired = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++fired;
}

int f(int x) pre<e2o>(x > 0) { return x; }

int main() {
  f(-1);                       // ignore -> identity -> ignore: no check, no fire
  if (fired != 0) __builtin_abort();
}
