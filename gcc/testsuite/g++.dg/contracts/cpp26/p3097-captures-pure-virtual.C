// P3097+P3098: Postcondition captures on a pure virtual function that has an
// out-of-line definition.  Verify the interface (pure-virtual declaration)
// capturing postcondition is checked both on polymorphic dispatch to an
// overrider and on a qualified call to the pure-virtual's own definition.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int state = 0;

struct Base {
  // Interface postcondition lives on the pure-virtual declaration.
  virtual int f()
    post [old = state] (r: r == old + 1) = 0;
  virtual ~Base() = default;
};

// Out-of-line definition of the pure virtual (agrees with its own interface).
int Base::f() { return ++state; }

struct Derived : Base {
  int f() override { return state += 2; }  // disagrees with interface post (+1)
};

int main() {
  Derived d;
  Base& b = d;

  // Polymorphic dispatch: captures old = 0, Derived::f sets state = 2.
  // Interface post: 2 == 0 + 1 -> false -> violation.
  state = 0;
  violation_count = 0;
  if (b.f() != 2) __builtin_abort();
  if (violation_count != 1) __builtin_abort();

  // Qualified call to the pure-virtual's definition: captures old = 2,
  // Base::f sets state = 3.  Interface post: 3 == 2 + 1 -> true -> no violation.
  violation_count = 0;
  if (b.Base::f() != 3) __builtin_abort();
  if (violation_count != 0) __builtin_abort();
}
