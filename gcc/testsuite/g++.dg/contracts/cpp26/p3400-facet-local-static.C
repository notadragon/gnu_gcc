// P3400: local_violation_label facet with static handler methods.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;
using std::contracts::labels::operator|;
using std::contracts::labels::empty_label;

static int local_calls = 0;
static int global_calls = 0;

// Static handler returning violation_handled.
struct static_handled_t {
  using assertion_control_object = static_handled_t;
  static violation_handled handle_contract_violation(const contract_violation&) {
    ++local_calls;
    return violation_handled::handled;
  }
};
constexpr static_handled_t static_handled{};

// Static handler returning not_handled.
struct static_not_handled_t {
  using assertion_control_object = static_not_handled_t;
  static violation_handled handle_contract_violation(const contract_violation&) {
    ++local_calls;
    return violation_handled::not_handled;
  }
};
constexpr static_not_handled_t static_not_handled{};

// Static handler returning void (treated as not_handled).
struct static_void_t {
  using assertion_control_object = static_void_t;
  static void handle_contract_violation(const contract_violation&) {
    ++local_calls;
  }
};
constexpr static_void_t static_void{};

// Non-static handler for combined label tests.
struct nonstatic_handled_t {
  using assertion_control_object = nonstatic_handled_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    ++local_calls;
    return violation_handled::handled;
  }
};
constexpr nonstatic_handled_t nonstatic_handled{};

void handle_contract_violation(const contract_violation&) {
  ++global_calls;
}

void test_static_handled(int x) pre<static_handled>(x > 0) { }
void test_static_not_handled(int x) pre<static_not_handled>(x > 0) { }
void test_static_void(int x) pre<static_void>(x > 0) { }

// Combined: static | empty.
void test_static_combined(int x) pre<(static_handled | empty_label)>(x > 0) { }

// Combined: empty | static.
void test_empty_static(int x) pre<(empty_label | static_handled)>(x > 0) { }

// Combined: non-static | static.
void test_nonstatic_static(int x)
  pre<(nonstatic_handled | static_not_handled)>(x > 0) { }

int main() {
  // Static handled: local called, global NOT called.
  local_calls = global_calls = 0;
  test_static_handled(-1);
  if (local_calls != 1 || global_calls != 0) __builtin_abort();

  // Static not_handled: local called, global called.
  local_calls = global_calls = 0;
  test_static_not_handled(-1);
  if (local_calls != 1 || global_calls != 1) __builtin_abort();

  // Static void: local called (void=not_handled), global called.
  local_calls = global_calls = 0;
  test_static_void(-1);
  if (local_calls != 1 || global_calls != 1) __builtin_abort();

  // Combined static | empty: static handles, no global.
  local_calls = global_calls = 0;
  test_static_combined(-1);
  if (local_calls != 1 || global_calls != 0) __builtin_abort();

  // Combined empty | static: static handles, no global.
  local_calls = global_calls = 0;
  test_empty_static(-1);
  if (local_calls != 1 || global_calls != 0) __builtin_abort();

  // Combined non-static | static: non-static called first, handles, skip static.
  local_calls = global_calls = 0;
  test_nonstatic_static(-1);
  if (local_calls != 1 || global_calls != 0) __builtin_abort();
}
