// P3595 x P3100: in the DYNAMIC path, a compute_semantic result of "assume"
// is gated by -fcontracts-allow-assume.  WITHOUT the flag, assume is removed
// from the effective allowed set, so the per-value transform T(enforce)=assume
// lands outside the set and becomes the sentinel: an unconditional enforced
// violation fires at RUNTIME (not a compile error), even though the predicate
// is true.  The label itself lists assume as allowed, so the ONLY thing keeping
// assume out is the missing flag.  Flag-on counterpart:
// p3595-dynamic-assume-allowed.C.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-assume-viol.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// compute_semantic maps enforce->assume; the compile-time default (observe
// from the config) maps to observe, which is allowed, so this is not a
// compile-time error -- the disallowed value only appears on the dynamic arm.
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

// Selector returns enforce; compute_semantic maps it to assume, which the
// flag-off gate removes from the allowed set -> sentinel.
evaluation_semantic p3595_sel_ev() { return evaluation_semantic::enforce; }

// Reaching the handler proves the unconditional enforced violation fired.
void handle_contract_violation(const std::contracts::contract_violation&) {
  std::exit(0);
}

void f(const int x) pre<to_assume>(x > 0) { }

int main() {
  f(1);                          // predicate TRUE, but the assume transform is
                                 // gated out, so the sentinel arm fires the
                                 // enforced violation unconditionally
  __builtin_abort ();            // unreachable: handler above should exit(0)
}
