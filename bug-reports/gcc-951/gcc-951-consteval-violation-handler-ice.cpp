// gcc-951-consteval-violation-handler-ice.cpp                       -*-C++-*-
//
// GCC-951 (PR128202): a consteval ::handle_contract_violation ICEs in
// build_call_a while the violation-handler wrapper is emitted.
//
//   g++ -std=c++26 -fcontracts -c gcc-951-consteval-violation-handler-ice.cpp
//
// Stock g++ trunk: internal compiler error: in build_call_a, at
// cp/call.cc:392 (in __tu_has_violation).  With
// -fcontract-evaluation-semantic=ignore or quick_enforce there is no call to
// the handler and no ICE.
//
// This branch: rejected at the declaration -- a consteval function is
// implicitly inline, and [dcl.fct.def.replace] forbids an inline
// replacement.
//
// PLAIN -fcontracts.

#include <contracts>

consteval void handle_contract_violation (const std::contracts::contract_violation &) {}

int main ()
{
  contract_assert (false);
}
