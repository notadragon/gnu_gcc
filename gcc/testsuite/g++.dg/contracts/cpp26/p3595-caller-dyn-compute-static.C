// P3595: compute_semantic is applied on the caller side (previously it was not).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-dyn-compute-static.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using std::contracts::evaluation_semantic;

// Label maps enforce -> observe, but is the identity on every other value
// (crucially, ignore -> ignore).  The callee-side default is ignore (CLI
// -fcontract-evaluation-semantic=ignore), so compute_semantic(ignore)=ignore
// leaves the callee side unchecked -- no callee-side handler fire.  The
// caller config resolves to enforce; with compute_semantic applied the
// effective caller-side semantic is observe (handler called, continue).
// Without the fix the caller side stays enforce (terminates).
struct enforce_to_observe_t {
  using assertion_control_object = enforce_to_observe_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const
  { return s == evaluation_semantic::enforce ? evaluation_semantic::observe : s; }
};
constexpr enforce_to_observe_t enforce_to_observe{};

static int fired = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++fired; }

int f(int x) pre<enforce_to_observe>(x > 0) { return x; }
int call() { return f(-1); }   // caller-side check

int main() { call(); if (fired != 1) __builtin_abort(); }
