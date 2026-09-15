// P3400: compute_semantic chaining order (left-to-right).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::labels::operator|;
using std::contracts::evaluation_semantic;

// A label that always returns a fixed semantic.
template<evaluation_semantic _S>
struct fixed_semantic_t {
  using assertion_control_object = fixed_semantic_t;
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic) const { return _S; }
};

constexpr fixed_semantic_t<evaluation_semantic::enforce> always_enforce{};
constexpr fixed_semantic_t<evaluation_semantic::observe> always_observe{};

// Chain: always_enforce | review
// LHS: enforce → enforce (always_enforce ignores input, returns enforce)
// RHS: enforce → observe (review transforms enforce to observe)
// Result: observe
void enforce_then_review(int x)
  pre<(always_enforce | review)>(x > 0)
{
}

// Chain: review | always_enforce
// LHS: enforce → observe (review transforms enforce to observe)
// RHS: observe → enforce (always_enforce ignores input, returns enforce)
// Result: enforce (terminates!)
// We can't test termination easily, so test the reverse direction:
// Use observe as configured semantic to avoid termination.

// Chain: always_observe | always_enforce
// LHS: any → observe (always_observe)
// RHS: any → enforce (always_enforce)
// Result: enforce (terminates)

// Chain: always_enforce | always_observe
// LHS: any → enforce
// RHS: any → observe
// Result: observe (continues)
void enforce_then_observe(int x)
  pre<(always_enforce | always_observe)>(x > 0)
{
}

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

int main() {
  // enforce_then_review: enforce → enforce → observe. Continues.
  enforce_then_review(-1);
  if (violations != 1) __builtin_abort();

  // enforce_then_observe: enforce → enforce → observe. Continues.
  enforce_then_observe(-1);
  if (violations != 2) __builtin_abort();
}
