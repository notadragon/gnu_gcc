// P3097: Qualified call bypasses virtual dispatch.
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
  void f(int x) override pre(x > 10) { }
};

int main() {
  Derived d;

  // Qualified call: d.Base::f(-1)
  // Only Base's callee-side contracts checked (no virtual dispatch wrapper).
  violation_count = 0;
  d.Base::f(-1);
  if (violation_count != 1) __builtin_abort();

  // d.Base::f(5) passes Base pre (x>0), no violations.
  violation_count = 0;
  d.Base::f(5);
  if (violation_count != 0) __builtin_abort();
}
