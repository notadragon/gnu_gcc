// P3400: operator| combines assertion-control labels.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct audit_t {
  using assertion_control_object = audit_t;
};
constexpr audit_t audit{};

struct user_review_t {
  using assertion_control_object = user_review_t;
};
constexpr user_review_t user_review{};

using std::contracts::labels::empty_label;
using std::contracts::labels::operator|;

// Combined labels satisfy assertion_control_object.
static_assert(requires { typename decltype(audit | user_review)::assertion_control_object; });
static_assert(requires { typename decltype(empty_label | audit)::assertion_control_object; });
static_assert(requires { typename decltype(audit | empty_label | user_review)::assertion_control_object; });

// Use combined label in assertion-control expression.
void f(int x)
  pre<(audit | user_review)>(x > 0)
{
}

// Chained combination via contract_control lookup.
constexpr auto combined = contract_control(audit | user_review | empty_label);
void g(int x)
  pre<combined>(x > 0)
{
}

// contract_control finds operator| through augmented lookup.
namespace my_labels {
  struct my_a_t { using assertion_control_object = my_a_t; };
  struct my_b_t { using assertion_control_object = my_b_t; };
  constexpr my_a_t my_a{};
  constexpr my_b_t my_b{};
}
using contract_control namespace my_labels;

constexpr auto my_combined = contract_control(my_a | my_b);
void h(int x)
  pre<my_combined>(x > 0)
{
}

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

int main() {
  f(-1);
  if (violations != 1) __builtin_abort();
  g(-1);
  if (violations != 2) __builtin_abort();
  h(-1);
  if (violations != 3) __builtin_abort();
  f(1); g(1); h(1);  // all pass
  if (violations != 3) __builtin_abort();
}
