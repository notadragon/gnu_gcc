// P3400: compute_semantic with ignore as the configured semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::labels::operator|;
using std::contracts::evaluation_semantic;

template<evaluation_semantic _S>
struct fixed_semantic_t {
  using assertion_control_object = fixed_semantic_t;
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic) const { return _S; }
};

constexpr fixed_semantic_t<evaluation_semantic::enforce> fixed_enforce{};
constexpr fixed_semantic_t<evaluation_semantic::observe> fixed_observe{};

static int violations = 0;
static bool should_not_be_called = false;

void handle_contract_violation(const std::contracts::contract_violation&) {
  if (should_not_be_called)
    __builtin_abort();
  ++violations;
}

// review: ignore -> ignore (review only transforms enforce/quick_enforce).
void with_review(int x)
  pre<review>(x > 0)
{
}

// fixed_observe: ignore -> observe (overrides to observe).
void with_always_observe(int x)
  pre<fixed_observe>(x > 0)
{
}

// fixed_enforce: ignore -> enforce (overrides to enforce, will terminate).
// We can't easily test termination, but we verify handler IS called
// by using fixed_observe instead (tested above).

// Chaining: fixed_observe | review
// ignore -> observe (fixed_observe) -> observe (review leaves observe alone)
void observe_then_review(int x)
  pre<(fixed_observe | review)>(x > 0)
{
}

int main() {
  // review doesn't change ignore -- contract not evaluated.
  should_not_be_called = true;
  with_review(-1);  // ignore: no evaluation, no handler
  should_not_be_called = false;
  if (violations != 0) __builtin_abort();

  // fixed_observe overrides ignore -> observe: handler called, continues.
  with_always_observe(-1);
  if (violations != 1) __builtin_abort();

  // fixed_observe | review: ignore -> observe -> observe.
  observe_then_review(-1);
  if (violations != 2) __builtin_abort();
}
