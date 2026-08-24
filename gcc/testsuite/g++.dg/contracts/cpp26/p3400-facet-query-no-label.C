// P3400: query_control_object returns nullptr when no queryable label.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::labels::empty_label;

static int handler_calls = 0;

// No label at all.
void test_no_label(int x) pre(x > 0) { }

// empty_label -- not queryable.
void test_empty_label(int x) pre<empty_label>(x > 0) { }

void handle_contract_violation(const contract_violation& v) {
  ++handler_calls;
  // query_control_object should return nullptr.
  static constexpr int some_key = 42;
  auto* r = v.query_control_object(&some_key);
  if (r != nullptr) __builtin_abort();
}

int main() {
  test_no_label(-1);
  if (handler_calls != 1) __builtin_abort();

  test_empty_label(-1);
  if (handler_calls != 2) __builtin_abort();

  return 0;
}
