// P3097+P3098: Interface postcondition with captures across virtual dispatch.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int shared_state = 0;

struct Base {
  virtual int f(int x)
    pre(x > 0)
    post [old_state = shared_state] (r: r == old_state + 1)
  { return shared_state; }
};

struct Derived : Base {
  int f(int x) override
    pre(x > -100)
  {
    shared_state += x;
    return shared_state;
  }
};

int main() {
  Derived d;
  Base& b = d;

  // shared_state starts at 0.  Call b.f(1):
  // Interface captures old_state = 0 before dispatch.
  // Derived::f sets shared_state = 1, returns 1.
  // Interface post: r == old_state + 1 -> 1 == 0 + 1 -> true.
  shared_state = 0;
  violation_count = 0;
  int r = b.f(1);
  if (violation_count != 0) __builtin_abort();
  if (r != 1) __builtin_abort();

  // Now shared_state = 1.  Call b.f(5):
  // Interface captures old_state = 1 before dispatch.
  // Derived::f sets shared_state = 6, returns 6.
  // Interface post: r == old_state + 1 -> 6 == 1 + 1 -> false (violation).
  violation_count = 0;
  r = b.f(5);
  if (violation_count != 1) __builtin_abort();
  if (r != 6) __builtin_abort();
}
