// P3097 x P3400: a label on the IMPLEMENTATION (override) contract of a virtual
// function governs the implementation check's semantic, independently of the
// interface (base) contract.  Here the override post has 'review' (enforce ->
// observe) and fails, while the base's plain (enforce) post passes -- so the
// implementation violation is observed and execution continues.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int handler_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++handler_count;
}

struct Base {
  virtual int f() post(r: r > 0) { return 1; }  // interface: enforce, passes
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
  if (handler_count != 1) __builtin_abort();  // implementation observed; reached
}
