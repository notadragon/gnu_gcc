// P3400: using contract_control namespace in function scope.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace my_labels {
  struct fn_label_t {
    using assertion_control_object = fn_label_t;
  };
  constexpr fn_label_t fn_label{};
}

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

void test_contract_assert() {
  using contract_control namespace my_labels;

  // contract_assert with unqualified label in function scope.
  contract_assert<fn_label>(true);   // passes
  contract_assert<fn_label>(false);  // violates
}

void test_lambda() {
  using contract_control namespace my_labels;

  // Lambda with pre/post using unqualified label from enclosing scope.
  auto checked_add = [](int a, int b)
    pre<fn_label>(a >= 0)
    pre<fn_label>(b >= 0)
    post<fn_label>(r: r >= 0)
  {
    return a + b;
  };

  checked_add(1, 2);   // passes
  checked_add(-1, 2);  // violates first pre
}

void test_contract_control_expr() {
  using contract_control namespace my_labels;

  // contract_control(expr) in the same function scope.
  constexpr auto local_label = contract_control(fn_label);
  contract_assert<local_label>(false);  // violates
}

int main() {
  test_contract_assert();
  if (violations != 1) __builtin_abort();

  test_lambda();
  if (violations != 2) __builtin_abort();

  test_contract_control_expr();
  if (violations != 3) __builtin_abort();
}
