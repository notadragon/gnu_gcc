// P3097+P3098+P3400: a 'review' label (compute_semantic: enforce -> observe) on
// a capturing interface postcondition of a virtual function.  The capture is
// still constructed as part of the postcondition unit, and because 'review'
// downgrades enforce to observe the violation is reported and execution
// CONTINUES rather than terminating.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int handler_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++handler_count;
}

static int state = 0;

struct Base {
  virtual int f()
    post<review> [old = state] (r: r == old + 1)
  { return ++state; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override { return state += 2; }  // disagrees with interface post (+1)
};

int main() {
  Derived d;
  Base& b = d;
  state = 0;
  handler_count = 0;

  // captures old = 0, Derived sets state = 2; interface post 2 == 0+1 -> false.
  // 'review' turned enforce into observe, so the handler runs and we continue.
  int r = b.f();
  if (r != 2) __builtin_abort();
  if (handler_count != 1) __builtin_abort();  // reaching here proves no terminate
}
