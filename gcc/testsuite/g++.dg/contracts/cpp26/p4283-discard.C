// P4283: Requires clause — unsatisfied contracts are discarded.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <concepts>
#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

template <typename T>
void check_positive(T x)
  pre requires(std::integral<T>) (x > 0)
{
}

template <typename T>
T identity(const T x)
  post requires(std::integral<T>) (r: r == x)
{
  return x;
}

template <typename T>
void assert_positive(T x) {
  contract_assert requires(std::integral<T>) (x > 0);
}

int main() {
  // Satisfied requires, violated predicate: violation triggered.
  violation_count = 0;
  check_positive(-1);
  if (violation_count != 1) __builtin_abort();

  // Unsatisfied requires (double): no contract, no violation.
  violation_count = 0;
  check_positive(-1.0);
  if (violation_count != 0) __builtin_abort();

  // Postcondition with satisfied requires.
  violation_count = 0;
  identity(42);
  if (violation_count != 0) __builtin_abort();

  // Postcondition with unsatisfied requires: discarded.
  violation_count = 0;
  identity(3.14);
  if (violation_count != 0) __builtin_abort();

  // contract_assert with satisfied requires, violated predicate.
  violation_count = 0;
  assert_positive(-5);
  if (violation_count != 1) __builtin_abort();

  // contract_assert with unsatisfied requires: discarded.
  violation_count = 0;
  assert_positive(-5.0);
  if (violation_count != 0) __builtin_abort();

  return 0;
}
