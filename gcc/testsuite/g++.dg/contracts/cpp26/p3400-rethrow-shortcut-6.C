// P3400: quick_enforce calls no violation handler at all, so there is no
// rethrowing handler to reason about and the try/catch stays.  Its reaction
// appears twice -- once in the catch, once on the predicate-false path.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fdump-tree-original" }
// { dg-additional-options "-fcontract-evaluation-semantic=quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

bool boom ();

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation (const contract_violation& v) const {
    if (v.detection_mode () == detection_mode::evaluation_exception)
      throw;
  }
};
constexpr rethrowing_t rethrowing{};

int f (int i) pre<rethrowing> (boom ()) { return i; }

// { dg-final { scan-tree-dump-times "__tu_quick_enforce_wrapper \\(\\)" 2 "original" } }
