// P3595: test -fcontract-configuration-file= with multiple files.
// Two config files loaded in order (repeatable, appended).
// First sets preconditions to ignore, second sets everything to observe.
// Since first-match wins, preconditions are ignored and asserts are observed.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-inline-pre.json" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-inline-default.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

void f(int x) pre(x > 0) { }

void g(int x) {
  contract_assert(x > 0);
}

int main() {
  // pre with ignore: no handler call
  f(-1);
  if (violations != 0) __builtin_abort();

  // contract_assert with observe: handler called, continues
  g(-1);
  if (violations != 1) __builtin_abort();
}
