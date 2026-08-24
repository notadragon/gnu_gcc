// P3097: Basic two-source checking for virtual function contracts.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static int check_count = 0;
static const char* checks[10];

void handle_contract_violation(const std::contracts::contract_violation& v) {
  if (check_count < 10)
    checks[check_count++] = v.comment();
}

struct Base {
  virtual int f(int x)
    pre(x > 0)
    post(r: r > 0)
  {
    return x;
  }
};

struct Derived : Base {
  int f(int x) override
    pre(x > -100)
    post(r: r > -100)
  {
    return x;
  }
};

int main() {
  Derived d;

  // Direct call through derived type: only implementation contracts.
  check_count = 0;
  d.f(5);
  if (check_count != 0) __builtin_abort();

  // Virtual call through base reference with valid input: no violations.
  Base& b = d;
  check_count = 0;
  b.f(5);
  if (check_count != 0) __builtin_abort();

  // Virtual call through base: interface pre (x > 0) fails with x = -50.
  // Implementation pre (x > -100) passes.
  // Derived returns -50, interface post (r > 0) also fails.
  check_count = 0;
  b.f(-50);
  if (check_count != 2) __builtin_abort();

  return 0;
}
