// P3400: both detection paths must be handed a data block that carries the
// label's facets.  GCC builds one block per contract and passes it to both
// entry points, which is why the defect Clang had here -- passing the bare
// global block to the _ex entry point, leaving a throwing predicate unable to
// reach the local handler or the query facet -- never existed on this side.
//
// This pins that structure so a future refactor cannot quietly split the two
// paths onto different blocks.  The behavioural half is in
// p3400-local-handler-exception-path.C.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4298 -fdump-tree-original" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstddef>

using std::contracts::contract_violation;
using std::contracts::violation_handled;

bool boom ();			// Not noexcept: the predicate might throw.

struct label_t {
  using assertion_control_object = label_t;
  violation_handled
  handle_contract_violation (const contract_violation&) const {
    return violation_handled::not_handled;
  }
  void* query (const void*, std::size_t) const { return nullptr; }
};
constexpr label_t label{};

int f (int i) pre<label> (boom ()) { return i; }

// Both entry points take a contract data block, and it is the same one -- the
// label's facets live in it, so a block reaching only one path would lose them
// on the other.
// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_enforce_pf \\(&\\*\\.Lcontract_data0\\)" 1 "original" } }
// { dg-final { scan-tree-dump-times "__cxa_contract_violation_pre_enforce_ex \\(&\\*\\.Lcontract_data0\\)" 1 "original" } }
