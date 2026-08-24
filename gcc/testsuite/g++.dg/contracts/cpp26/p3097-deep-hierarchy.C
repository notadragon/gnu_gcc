// P3097: Deep inheritance — only static and dynamic type contracts checked.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int log_idx = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++log_idx;
}

struct X {
  virtual void f(int n) pre(n > 0) { }
};

struct Y : X {
  void f(int n) override pre(n > -10) { }
};

struct Z : Y {
  void f(int n) override pre(n > -100) { }
};

int main() {
  Z z;

  // Call through X& — interface is X, implementation is Z.
  // Y's contracts are NOT checked (intermediate).
  log_idx = 0;
  X& x = z;
  x.f(-50);  // X pre fails (n>0), Z pre passes (n>-100)
  if (log_idx != 1) __builtin_abort();

  // Call through Y& — interface is Y, implementation is Z.
  log_idx = 0;
  Y& y = z;
  y.f(-50);  // Y pre fails (n>-10), Z pre passes (n>-100)
  if (log_idx != 1) __builtin_abort();

  // Call through Z& — interface == implementation.
  log_idx = 0;
  z.f(-50);  // Z pre passes (n>-100), checked once
  if (log_idx != 0) __builtin_abort();
}
