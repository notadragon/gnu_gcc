// P3595: caller-side dynamic selection with "provideweak": false -- the
// compiler emits no weak default definition for the selector; a strong
// user-supplied selector must still link and drive the caller-side check.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-dyn-noweak.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using std::contracts::evaluation_semantic;

static int fired = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++fired; }

// Strong definition: the only definition, since provideweak is false.
evaluation_semantic p3595_caller_sel_nw() { return evaluation_semantic::observe; }

int f(int x) pre(x > 0) { return x; }   // callee-side ignored (CLI default)
int call() { return f(-1); }            // caller-side dynamic wrapper

int main() {
  call();
  if (fired != 1) __builtin_abort();     // selector chose observe caller-side
}
