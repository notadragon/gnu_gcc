// P3400: local_violation_label facet — local violation handlers.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;
using std::contracts::labels::operator|;

static int local_calls = 0;
static int global_calls = 0;

// Label that handles — global handler NOT called.
struct handled_label_t {
  using assertion_control_object = handled_label_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    ++local_calls;
    return violation_handled::handled;
  }
};
constexpr handled_label_t handled_label{};

// Label that does NOT handle — global handler IS called.
struct not_handled_label_t {
  using assertion_control_object = not_handled_label_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    ++local_calls;
    return violation_handled::not_handled;
  }
};
constexpr not_handled_label_t not_handled_label{};

// Label with void return — treated as not_handled, global handler called.
struct void_handler_label_t {
  using assertion_control_object = void_handler_label_t;
  void handle_contract_violation(const contract_violation&) const {
    ++local_calls;
  }
};
constexpr void_handler_label_t void_handler_label{};

void handle_contract_violation(const contract_violation&) {
  ++global_calls;
}

void test_handled(int x) pre<handled_label>(x > 0) { }
void test_not_handled(int x) pre<not_handled_label>(x > 0) { }
void test_void_handler(int x) pre<void_handler_label>(x > 0) { }

int main() {
  // handled_label: local called, global NOT called.
  test_handled(-1);
  if (local_calls != 1 || global_calls != 0) __builtin_abort();

  // not_handled_label: local called, then global called.
  test_not_handled(-1);
  if (local_calls != 2 || global_calls != 1) __builtin_abort();

  // void_handler_label: local called (void=not_handled), global called.
  test_void_handler(-1);
  if (local_calls != 3 || global_calls != 2) __builtin_abort();
}
