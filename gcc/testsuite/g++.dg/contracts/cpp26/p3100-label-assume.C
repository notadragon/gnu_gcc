// P3100: interaction of the "assume" semantic with allowed_semantics labels,
// with -fcontracts-allow-assume.  The configured semantic is "assume"; each
// label intersects the allowed set, and resolution adjusts into it.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p3100 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// Allows ignore + observe (assume is not listed).
struct ig_ob_t {
  using assertion_control_object = ig_ob_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::ignore, evaluation_semantic::observe};
};
constexpr ig_ob_t ig_ob{};

// Allows observe only (neither assume nor ignore).
struct ob_t {
  using assertion_control_object = ob_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe};
};
constexpr ob_t ob{};

static int side = 0;
static int viol = 0;
bool chk(int x) { ++side; return x > 0; }
void handle_contract_violation(const std::contracts::contract_violation&)
{ ++viol; }

// assume not in {ignore, observe}, but ignore is -> ignore.
// No predicate evaluation, no violation.
void f_ignore(int x) pre<ig_ob>(chk(x)) {}

// assume not in {observe}, ignore not in {observe} -> falls to observe.
// Predicate evaluated, violation reported.
void f_observe(int x) pre<ob>(chk(x)) {}

int main()
{
  f_ignore(-1);
  if (side != 0 || viol != 0) __builtin_abort();   // ignore: nothing happened

  f_observe(-1);
  if (side != 1 || viol != 1) __builtin_abort();   // observe: evaluated + reported
}
