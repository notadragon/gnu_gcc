// P3595 x templates: a configuration entry that matches the source location of a
// function template's contract applies to every instantiation of that template.
// The template's precondition line is configured to observe while the base
// semantic is enforce; both the int and double instantiations observe (rather
// than terminate) when the precondition fails.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-template.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

template <typename T>
T f(T x)
  pre(x > T{})   // this line is configured to observe
{ return x; }

int main() {
  violations = 0;
  (void) f(-1);     // int instantiation: pre fails -> observed
  (void) f(-1.0);   // double instantiation: pre fails -> observed
  if (violations != 2) __builtin_abort();  // both observed, neither terminated
}
