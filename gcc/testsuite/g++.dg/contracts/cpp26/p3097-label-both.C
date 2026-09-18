// P3097 x P3400: distinct labels on the interface and the implementation
// contracts of a virtual function are each honored on their own check.  Here
// both the base (interface) and override (implementation) posts have 'review'
// (enforce -> observe) and both fail, so both checks are observed and BOTH
// violations are reported while execution continues.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int handler_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++handler_count;
}

struct Base {
  virtual int f() post<review>(r: r > 100) { return 1; }  // interface: review->observe
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override post<review>(r: r > 100) { return 5; }  // impl: review->observe
};

int main() {
  Derived d;
  Base& b = d;
  handler_count = 0;
  int r = b.f();
  if (r != 5) __builtin_abort();
  if (handler_count != 2) __builtin_abort();  // both interface and impl observed
}
