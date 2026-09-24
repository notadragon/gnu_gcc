// P3097+P3098: Captures destroyed on exception from virtual dispatch.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int dtor_count = 0;

struct Tracker {
  Tracker() = default;
  Tracker(const Tracker&) { }
  ~Tracker() { ++dtor_count; }
};

void handle_contract_violation(const std::contracts::contract_violation&) { }

static Tracker global_tracker;

struct Base {
  virtual int f()
    post [t = global_tracker] (r: r > 0)
  { return 1; }
};

struct Derived : Base {
  int f() override {
    throw 42;
  }
};

int main() {
  Derived d;
  Base& b = d;

  dtor_count = 0;
  try {
    b.f();
    __builtin_abort();  // should not reach here
  } catch (int) {
    // Capture 't' should have been destroyed during unwinding.
  }
  // Verify at least one Tracker destructor ran (the capture copy).
  if (dtor_count < 1) __builtin_abort();
}
