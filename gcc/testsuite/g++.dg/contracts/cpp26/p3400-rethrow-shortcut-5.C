// P3400: the rethrow shortcut is restricted to enforce and observe.  The
// noexcept semantics (D4298) exist to guarantee nothing propagates out of a
// check, so their try/catch must survive even for a rethrowing handler.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4298 -fdump-tree-original" }
// { dg-additional-options "-fcontract-evaluation-semantic=noexcept_observe" }
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

// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_noexcept_observe_pf_noexcept" 1 "original" } }
// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_noexcept_observe_ex_noexcept" 1 "original" } }
