// P3400: allowed_semantics facet — restricts and adjusts evaluation semantics.
// Configured semantic is enforce. Labels restrict what's allowed.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using std::contracts::labels::operator|;

// Label that allows only observe and ignore.
struct observe_or_ignore_t {
  using assertion_control_object = observe_or_ignore_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe, evaluation_semantic::ignore};
};
constexpr observe_or_ignore_t observe_or_ignore{};

// Label that allows only enforce and quick_enforce (terminating).
struct terminating_t {
  using assertion_control_object = terminating_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::enforce, evaluation_semantic::quick_enforce};
};
constexpr terminating_t terminating{};

// Label that allows everything (no restriction).
struct any_t {
  using assertion_control_object = any_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    evaluation_semantic_set::all();
};
constexpr any_t any_label{};

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// With observe_or_ignore: configured enforce is adjusted to observe.
// Handler called, execution continues.
void with_observe_only(int x)
  pre<observe_or_ignore>(x > 0)
{
}

// With terminating: configured enforce is allowed, no adjustment.
// This will terminate on violation so we only call with passing input.
void with_terminating(int x)
  pre<terminating>(x > 0)
{
}

// With any_label: enforce is allowed, no adjustment.
void with_any(int x)
  pre<any_label>(x > 0)
{
}

// Combined: observe_or_ignore | any_label → intersection = observe_or_ignore.
// Adjusted to observe.
void combined_restrict(int x)
  pre<(observe_or_ignore | any_label)>(x > 0)
{
}

// Combined: terminating | observe_or_ignore → intersection = empty set!
// This should be a compile error (tested separately).

int main() {
  // observe_or_ignore: enforce adjusted → observe. Continues.
  with_observe_only(-1);
  if (violations != 1) __builtin_abort();

  // terminating: enforce is allowed. Don't violate (would terminate).
  with_terminating(1);

  // any_label: enforce is allowed. Don't violate (would terminate).
  with_any(1);

  // combined_restrict: observe_or_ignore wins. Continues.
  combined_restrict(-1);
  if (violations != 2) __builtin_abort();
}
