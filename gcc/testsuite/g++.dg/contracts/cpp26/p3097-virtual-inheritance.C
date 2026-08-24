// P3097: Virtual base classes with contracts.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

struct VBase {
  virtual void f(int x) pre(x > 0) { }
};

struct Mid1 : virtual VBase {
  void f(int x) override pre(x > -10) { }
};

struct Mid2 : virtual VBase {
  void f(int x) override pre(x > -20) { }
};

struct Bottom : Mid1, Mid2 {
  void f(int x) override pre(x > -100) { }
};

int main() {
  Bottom bot;
  VBase& vb = bot;

  // Call through VBase&: interface = VBase, implementation = Bottom.
  violation_count = 0;
  vb.f(-50);  // VBase pre fails (x>0), Bottom pre passes (x>-100)
  if (violation_count != 1) __builtin_abort();

  violation_count = 0;
  vb.f(5);  // both pass
  if (violation_count != 0) __builtin_abort();
}
