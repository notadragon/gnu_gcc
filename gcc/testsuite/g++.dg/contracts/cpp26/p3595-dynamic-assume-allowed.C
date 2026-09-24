// P3595 x P3100: in the DYNAMIC path, a compute_semantic result of "assume"
// is gated by -fcontracts-allow-assume.  WITH the flag, assume stays in the
// effective allowed set, so the per-value transform T(enforce)=assume resolves
// to assume, which codegens like ignore: no check, the predicate is not
// evaluated, and no violation fires.  Same source and config as the flag-off
// counterpart p3595-dynamic-assume-viol.C -- only the flag differs.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-allow-assume" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-assume-allowed.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct to_assume_t {
  using assertion_control_object = to_assume_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe, evaluation_semantic::enforce,
     evaluation_semantic::assume};
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic s) const
  {
    return s == evaluation_semantic::enforce
      ? evaluation_semantic::assume
      : s;
  }
};
constexpr to_assume_t to_assume{};

// Selector returns enforce; compute_semantic maps it to assume, which is in the
// allowed set once -fcontracts-allow-assume is given.
evaluation_semantic p3595_sel_ev() { return evaluation_semantic::enforce; }

static int side = 0;
bool chk(int x) { ++side; return x > 0; }
// assume must not report a violation.
void handle_contract_violation(const std::contracts::contract_violation&) {
  __builtin_abort();
}

void f(int x) pre<to_assume>(chk(x)) { }

int main() {
  f(-1);                         // enforce -> assume (allowed): no check,
                                 // predicate skipped, no violation
  if (side != 0) __builtin_abort();
}
