// P3400: Test all permutations of allowed_semantics and compute_semantic facets.
// Configured semantic: enforce.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using std::contracts::labels::operator|;

// --- Labels with NEITHER facet ---
struct plain_t { using assertion_control_object = plain_t; };
constexpr plain_t plain{};

// --- Labels with ONLY allowed_semantics ---
struct only_observe_t {
  using assertion_control_object = only_observe_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe};
};
constexpr only_observe_t only_observe{};

struct enforce_ok_t {
  using assertion_control_object = enforce_ok_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::enforce, evaluation_semantic::observe};
};
constexpr enforce_ok_t enforce_ok{};

// --- Labels with ONLY compute_semantic ---
// (review is provided by the library)

// --- Labels with BOTH facets ---
struct observe_and_compute_t {
  using assertion_control_object = observe_and_compute_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe, evaluation_semantic::ignore};
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic) const
  { return evaluation_semantic::observe; }
};
constexpr observe_and_compute_t observe_and_compute{};

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// Case 1: Neither facet. Configured enforce is used directly.
// (Would terminate on violation — only test passing case.)
void case_neither(int x) pre<plain>(x > 0) { }

// Case 2: Only allowed_semantics, enforce is allowed.
// No adjustment needed.
void case_allowed_ok(int x) pre<enforce_ok>(x > 0) { }

// Case 3: Only allowed_semantics, enforce NOT allowed → adjusted to observe.
void case_allowed_adjust(int x) pre<only_observe>(x > 0) { }

// Case 4: Only compute_semantic (review: enforce → observe).
void case_compute_only(int x) pre<review>(x > 0) { }

// Case 5: Both facets. enforce NOT in allowed → adjusted to observe.
// Then compute_semantic(observe) → observe (identity for this label).
void case_both(int x) pre<observe_and_compute>(x > 0) { }

// Case 6: Combined labels.
// enforce_ok | review: allowed={enforce,observe}. enforce allowed, no adjust.
// Then review: enforce → observe. observe is in allowed. OK.
void case_combined_allowed_and_compute(int x)
  pre<(enforce_ok | review)>(x > 0)
{
}

int main() {
  // Case 1: enforce, don't violate (would terminate).
  case_neither(1);

  // Case 2: enforce allowed, don't violate.
  case_allowed_ok(1);

  // Case 3: adjusted to observe, violate — handler called, continues.
  case_allowed_adjust(-1);
  if (violations != 1) __builtin_abort();

  // Case 4: compute_semantic: enforce→observe. Continues.
  case_compute_only(-1);
  if (violations != 2) __builtin_abort();

  // Case 5: both facets. Adjusted+computed to observe. Continues.
  case_both(-1);
  if (violations != 3) __builtin_abort();

  // Case 6: combined. enforce allowed, review computes to observe. Continues.
  case_combined_allowed_and_compute(-1);
  if (violations != 4) __builtin_abort();
}
