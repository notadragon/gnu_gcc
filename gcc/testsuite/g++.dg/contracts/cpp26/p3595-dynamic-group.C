// P3595 dynamic selection via a group-label fallback.  The contract carries a
// P3400 identification-label group ("safety") and the config entry is keyed on
// that group with a "dynamic" output.  The dynamic scan must match the same
// group-keyed entry the scalar scan matched; otherwise the query carries no
// group, the entry does not match, and the selector is never consulted.
// Default is "ignore"; the runtime selector returns "observe", so the failing
// precondition is handled once and execution continues.
// (Clang mirror: clang/test/Contracts/Runnable/p3595-dynamic-group.cpp, which
// uses the [[clang::contract_group]] attribute; GCC uses the P3400 group label.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-group.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using std::contracts::evaluation_semantic;

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

evaluation_semantic p3595_group_sel() { return evaluation_semantic::observe; }

void f(const int x) pre<"safety"group>(x > 0) { }

int main() {
  f(-1);                         // observe -> handler called, continue
  if (violations != 1) __builtin_abort();
  f(1);                          // no violation
  if (violations != 1) __builtin_abort();
}
