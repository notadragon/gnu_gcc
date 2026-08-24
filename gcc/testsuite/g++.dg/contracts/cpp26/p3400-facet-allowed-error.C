// P3400: allowed_semantics facet — errors.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using std::contracts::labels::operator|;

// Empty allowed set.
struct nothing_allowed_t {
  using assertion_control_object = nothing_allowed_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    evaluation_semantic_set::none();
};
constexpr nothing_allowed_t nothing{};

// Error: no semantics allowed.
void f(int x)
  pre<nothing>(x > 0)  // { dg-error "allows no evaluation semantics" }
{
}

// Allowed only observe, but compute_semantic returns enforce.
struct bad_compute_t {
  using assertion_control_object = bad_compute_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe};
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic) const
  { return evaluation_semantic::enforce; }
};
constexpr bad_compute_t bad_compute{};

// Error: compute_semantic result (enforce) not in allowed set (observe only).
void g(int x)
  pre<bad_compute>(x > 0)  // { dg-error "compute_semantic. result is not in the allowed" }
{
}

// Two labels whose allowed_semantics are disjoint.  Combining them with
// operator| intersects the sets to the empty set, which is ill-formed for the
// same reason as the explicitly-empty set above.  This is the empty-intersection
// case that p3400-facet-allowed.C notes is "tested separately".
struct observe_or_ignore_t {
  using assertion_control_object = observe_or_ignore_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe, evaluation_semantic::ignore};
};
constexpr observe_or_ignore_t observe_or_ignore{};

struct terminating_t {
  using assertion_control_object = terminating_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::enforce, evaluation_semantic::quick_enforce};
};
constexpr terminating_t terminating{};

// Error: intersection of the two allowed sets is empty.
void h(int x)
  pre<(terminating | observe_or_ignore)>(x > 0)  // { dg-error "allows no evaluation semantics" }
{
}
