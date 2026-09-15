// P3097: Override with no contracts — interface contracts still checked.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

struct Base {
  virtual int f(int x) pre(x > 0) post(r: r > 0) { return x; }
};

struct Derived : Base {
  int f(int x) override { return x; }  // no contracts
};

int main() {
  Derived d;
  Base& b = d;

  violation_count = 0;
  b.f(5);
  if (violation_count != 0) __builtin_abort();  // all pass

  violation_count = 0;
  b.f(-1);
  // Interface pre (x>0) fails.  No implementation pre.
  // Implementation returns -1, interface post (r>0) fails.
  if (violation_count != 2) __builtin_abort();
}
