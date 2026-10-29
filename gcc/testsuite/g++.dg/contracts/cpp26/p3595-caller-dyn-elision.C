// P3595: caller-side dynamic selection whose compile-time default is
// "ignore" -- without forcing the wrapper active (Hook C in the caller-side
// dynamic design), a wrapper whose only caller-side contract defaults to
// ignore would be elided entirely and the selector would never run.  With
// the force-active behavior, the wrapper is emitted and the selector's
// run-time choice (observe) drives the check.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-dyn-elision.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using std::contracts::evaluation_semantic;

static int fired = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++fired; }

// Selector overrides the ignore compile-time default at run time.
evaluation_semantic p3595_caller_sel_elision() { return evaluation_semantic::observe; }

int f(int x) pre(x > 0) { return x; }   // callee-side ignored (CLI default)
int call() { return f(-1); }            // caller-side dynamic wrapper

int main() {
  call();
  // Would be 0 if the wrapper were elided because the compile-time default
  // is "ignore"; the dynamic descriptor must force it active.
  if (fired != 1) __builtin_abort();
}
