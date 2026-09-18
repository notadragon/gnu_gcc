// P3400: Local handler chaining with combined labels.
// RHS handle_contract_violation is called first; LHS only if RHS didn't handle.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;
using std::contracts::labels::operator|;

static int lhs_calls = 0;
static int rhs_calls = 0;
static int global_calls = 0;

struct lhs_handled_t {
  using assertion_control_object = lhs_handled_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    ++lhs_calls;
    return violation_handled::handled;
  }
};
constexpr lhs_handled_t lhs_handled{};

struct rhs_handled_t {
  using assertion_control_object = rhs_handled_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    ++rhs_calls;
    return violation_handled::handled;
  }
};
constexpr rhs_handled_t rhs_handled{};

struct rhs_not_handled_t {
  using assertion_control_object = rhs_not_handled_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    ++rhs_calls;
    return violation_handled::not_handled;
  }
};
constexpr rhs_not_handled_t rhs_not_handled{};

void handle_contract_violation(const contract_violation&) {
  ++global_calls;
}

// Case 1: LHS handles, RHS handles → LHS called first, returns handled, skip RHS.
void case_both_handle(int x)
  pre<(lhs_handled | rhs_handled)>(x > 0)
{
}

// Case 2: LHS doesn't handle, RHS handles → LHS called, not handled,
// then RHS called, handled.
void case_lhs_not_rhs_yes(int x)
  pre<(rhs_not_handled | rhs_handled)>(x > 0)
{
}

// Case 3: Only LHS has handler (RHS is empty_label).
void case_only_lhs(int x)
  pre<(lhs_handled | empty_label)>(x > 0)
{
}

// Case 4: Only RHS has handler.
void case_only_rhs(int x)
  pre<(empty_label | rhs_handled)>(x > 0)
{
}

// Case 5: Neither has handler (empty | empty). Global handler called.
void case_neither(int x)
  pre<(empty_label | empty_label)>(x > 0)
{
}

int main() {
  // Case 1: both handle. LHS called, handled. No RHS, no global.
  lhs_calls = rhs_calls = global_calls = 0;
  case_both_handle(-1);
  if (lhs_calls != 1 || rhs_calls != 0 || global_calls != 0) __builtin_abort();

  // Case 2: LHS not handled, then RHS handled. No global.
  lhs_calls = rhs_calls = global_calls = 0;
  case_lhs_not_rhs_yes(-1);
  // __combined_label calls LHS first (rhs_not_handled), then RHS (rhs_handled)
  if (rhs_calls != 2 || global_calls != 0) __builtin_abort();

  // Case 3: only LHS. Handled, no global.
  lhs_calls = rhs_calls = global_calls = 0;
  case_only_lhs(-1);
  if (lhs_calls != 1 || global_calls != 0) __builtin_abort();

  // Case 4: only RHS. Handled, no global.
  lhs_calls = rhs_calls = global_calls = 0;
  case_only_rhs(-1);
  if (rhs_calls != 1 || global_calls != 0) __builtin_abort();

  // Case 5: neither. Global called.
  lhs_calls = rhs_calls = global_calls = 0;
  case_neither(-1);
  if (lhs_calls != 0 && rhs_calls != 0 || global_calls != 1) __builtin_abort();
}
