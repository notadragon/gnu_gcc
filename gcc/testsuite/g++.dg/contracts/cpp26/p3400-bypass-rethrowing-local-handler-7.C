// P3400: -fno-contract-bypass-rethrowing-local-handler turns the optimization off, so a
// handler that would otherwise qualify gets its try/catch back.  Same source
// as p3400-bypass-rethrowing-local-handler-3.C case 1, opposite expectation.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fdump-tree-original" }
// { dg-additional-options "-fno-contract-bypass-rethrowing-local-handler" }
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

// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_enforce_pf" 1 "original" } }
// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_enforce_ex" 1 "original" } }
