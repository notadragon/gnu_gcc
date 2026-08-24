// P4283: Requires clauses in various template contexts.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <concepts>
#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

// Class template with requires on member function contracts.
template <typename T>
struct Container {
  T value;

  void set(T v)
    pre requires(std::totally_ordered<T>) (v > value)
  {
    value = v;
  }

  T get() const
    post requires(std::integral<T>) (r: r >= 0)
  {
    return value;
  }
};

// Function template with multiple contracts, only some constrained.
template <typename T>
T clamp_positive(const T x)
  pre (true)
  pre requires(std::signed_integral<T>) (x > -1000)
  post requires(std::integral<T>) (r: r >= 0)
{
  if (x < T{}) return T{};
  return x;
}

// Member function template.
struct Validator {
  template <typename T>
  void check(T x)
    pre requires(std::integral<T>) (x != 0)
  {}
};

int main() {
  // Class template: int — contracts active (both totally_ordered and integral).
  Container<int> ci{0};
  violation_count = 0;
  ci.set(10);       // pre satisfied (10 > 0)
  if (violation_count != 0) __builtin_abort();
  ci.get();         // post satisfied (10 >= 0)
  if (violation_count != 0) __builtin_abort();

  // Class template: double — totally_ordered satisfied but not integral.
  Container<double> cd{0.0};
  violation_count = 0;
  cd.set(1.5);      // pre active (totally_ordered<double>), satisfied (1.5 > 0.0)
  if (violation_count != 0) __builtin_abort();
  cd.get();         // post discarded (not integral<double>)
  if (violation_count != 0) __builtin_abort();

  // Multiple contracts, mixed constrained/unconstrained: int.
  violation_count = 0;
  clamp_positive(5);   // all satisfied
  if (violation_count != 0) __builtin_abort();

  // Multiple contracts: unsigned int — signed_integral not satisfied, discarded.
  violation_count = 0;
  clamp_positive(5u);  // pre(true) active, pre requires discarded, post active (r>=0 ok)
  if (violation_count != 0) __builtin_abort();

  // Multiple contracts: double — signed_integral and integral both not satisfied.
  violation_count = 0;
  clamp_positive(3.14); // only pre(true) active
  if (violation_count != 0) __builtin_abort();

  // Member function template.
  Validator v;
  violation_count = 0;
  v.check(42);      // integral: active, satisfied
  if (violation_count != 0) __builtin_abort();
  v.check(3.14);    // not integral: discarded
  if (violation_count != 0) __builtin_abort();

  // Violated predicates with satisfied requires.
  violation_count = 0;
  ci.value = 100;
  ci.set(50);       // pre active, violated (50 > 100 is false)
  if (violation_count != 1) __builtin_abort();

  violation_count = 0;
  Container<int> cn{-5};
  cn.get();         // post active, violated (-5 >= 0 is false)
  if (violation_count != 1) __builtin_abort();

  return 0;
}
