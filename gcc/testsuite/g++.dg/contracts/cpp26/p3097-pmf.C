// P3097: Pointer-to-member-function — only implementation contracts checked.
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
  void f(int x) override pre(x > -100) { }
};

int main() {
  Derived d;
  void (Base::*pmf)(int) = &Base::f;

  // PMF call: only implementation (Derived) contracts checked.
  // x=5 passes Derived pre (x>-100).
  violation_count = 0;
  (d.*pmf)(5);
  if (violation_count != 0) __builtin_abort();

  // x=-50: Derived pre passes (x>-100).
  // Base pre (x>0) should NOT be checked via PMF.
  violation_count = 0;
  (d.*pmf)(-50);
  if (violation_count != 0) __builtin_abort();

  // x=-200: Derived pre fails (x>-100).
  violation_count = 0;
  (d.*pmf)(-200);
  if (violation_count != 1) __builtin_abort();
}
