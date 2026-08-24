// P3595: test namespace prefix matching in JSON config.
// Config sets mylib::internal to ignore, mylib to observe, default to observe.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-namespace.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

namespace mylib {
  void f(int x) pre(x > 0) { }

  namespace internal {
    void g(int x) pre(x > 0) { }
  }
}

void h(int x) pre(x > 0) { }

int main() {
  // mylib::internal -> ignore: no handler call
  mylib::internal::g(-1);
  if (violations != 0) __builtin_abort();

  // mylib -> observe: handler called, continues
  mylib::f(-1);
  if (violations != 1) __builtin_abort();

  // global -> observe (catch-all): handler called, continues
  h(-1);
  if (violations != 2) __builtin_abort();
}
