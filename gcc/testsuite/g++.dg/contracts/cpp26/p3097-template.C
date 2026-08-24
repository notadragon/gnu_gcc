// P3097 x templates: contracts on a virtual member function of a class template.
// The interface pre/post are checked across virtual dispatch for each
// instantiation.  (No captures here -- capture-in-class-template-member is
// tracked separately as BUG-1.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violation_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

template <typename T>
struct Base {
  virtual T f(T x)
    pre(x > T{})
    post(r: r > T{})
  { return x; }
  virtual ~Base() = default;
};

template <typename T>
struct Derived : Base<T> {
  T f(T x) override { return x - 1; }  // may violate interface post
};

int main() {
  Derived<int> d;
  Base<int>& b = d;

  violation_count = 0;
  if (b.f(5) != 4) __builtin_abort();     // pre 5>0 ok; post 4>0 ok
  if (violation_count != 0) __builtin_abort();

  violation_count = 0;
  if (b.f(1) != 0) __builtin_abort();     // post 0>0 false -> 1 violation
  if (violation_count != 1) __builtin_abort();

  violation_count = 0;
  if (b.f(0) != -1) __builtin_abort();    // pre 0>0 false + post -1>0 false -> 2
  if (violation_count != 2) __builtin_abort();

  // Second instantiation compiles and runs.
  Derived<double> dd;
  Base<double>& bd = dd;
  violation_count = 0;
  if (bd.f(5.0) != 4.0) __builtin_abort();
  if (violation_count != 0) __builtin_abort();
}
