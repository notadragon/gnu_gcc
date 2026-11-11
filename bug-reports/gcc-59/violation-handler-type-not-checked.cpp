// g++ -std=c++26 -fcontracts -c violation-handler-type-not-checked.cpp
//
// [basic.contract.handler]: "The contract-violation handler shall have a
// single parameter of type 'lvalue reference to const
// std::contracts::contract_violation' and shall return void."  That is a
// diagnosable rule (no "no diagnostic required").  Each declaration below
// breaks it; GCC 16.1, 16.2 and trunk accept all of them silently.
// Compile each block separately (-DCASE=1..3), or all at once: none is
// diagnosed.

#include <contracts>

#if !defined(CASE) || CASE == 1
// Wrong parameter type and wrong return type.
int handle_contract_violation (int) { return 0; }
#endif

#if !defined(CASE) || CASE == 2
// Reference to const volatile, not to const.
void handle_contract_violation (const volatile std::contracts::contract_violation&) {}
#endif

#if !defined(CASE) || CASE == 3
// Two parameters.
void handle_contract_violation (const std::contracts::contract_violation&, int) {}
#endif
