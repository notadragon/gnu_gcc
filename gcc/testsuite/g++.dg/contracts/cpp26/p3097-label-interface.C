// P3097 x P3400: a label on the INTERFACE (base) contract of a virtual function
// governs the interface check's semantic, independently of the implementation
// (override) contract.  Here the base post has 'review' (enforce -> observe) and
// fails, while the override's plain (enforce) post passes -- so the interface
// violation is observed and execution continues rather than terminating.
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
  int f() override post(r: r > 0) { return 5; }  // implementation: enforce, passes
};

int main() {
  Derived d;
  Base& b = d;
  handler_count = 0;
  int r = b.f();
  if (r != 5) __builtin_abort();
  if (handler_count != 1) __builtin_abort();  // interface observed; reached => no terminate
}
