// P3097: Devirtualized call — contracts checked once.
// When static == dynamic type, no virtual dispatch wrapper is used.
// Only the callee-side (implementation) contracts are checked.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

struct Base {
  virtual void f(int x) pre(x > 0) { }
};

struct Derived : Base {
  void f(int x) override pre(x > 0) { }
};

int main() {
  Derived d;

  // Direct call on derived — static == dynamic type.
  // Only Derived's callee-side contracts checked (once).
  violation_count = 0;
  d.f(-1);
  if (violation_count != 1) __builtin_abort();

  // Valid call — no violations.
  violation_count = 0;
  d.f(5);
  if (violation_count != 0) __builtin_abort();
}
