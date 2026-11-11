// The contract-violation handler's parameter must be exactly "lvalue
// reference to const std::contracts::contract_violation"
// ([basic.contract.handler]), a diagnosable rule, so a reference to const
// volatile is rejected.  Stock GCC checks the handler's type not at all
// (GCC-59 in this repository's bug-reports/, fixed here).
//
// Mirror: clang/test/Contracts/violation-handler-cv-param.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

void handle_contract_violation (const volatile std::contracts::contract_violation&) {} // { dg-error "handle_contract_violation" }
