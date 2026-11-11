// P3097+P3098: Both interface and implementation postconditions have captures.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int counter = 0;

struct Base {
  virtual int f()
    post [old_c = counter] (r: r == old_c + 1)
  { return ++counter; }
};

struct Derived : Base {
  int f() override
    post [old_c = counter] (r: r == old_c + 2)
  {
    counter += 2;
    return counter;
  }
};

int main() {
  Derived d;
  Base& b = d;

  // counter starts at 0.
  // Interface captures old_c = 0.
  // Implementation captures old_c = 0.
  // Derived::f: counter becomes 2, returns 2.
  // Implementation post: r == old_c + 2 -> 2 == 0 + 2 -> true.
  // Interface post: r == old_c + 1 -> 2 == 0 + 1 -> false (violation).
  counter = 0;
  violation_count = 0;
  int r = b.f();
  if (r != 2) __builtin_abort();
  if (violation_count != 1) __builtin_abort();  // only interface post fails
}
