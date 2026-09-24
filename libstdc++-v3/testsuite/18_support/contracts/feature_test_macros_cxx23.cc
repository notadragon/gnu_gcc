// { dg-options "-std=gnu++23 -fcontracts-p3850" }
// { dg-do compile }

// The C++23 half of feature_test_macros_cxx20.cc; see that file and
// bug-reports/gcc-50/ for why the dialect is pinned and what used to fail.
// C++23 is the dialect the bug was reported against, and is covered here
// rather than inferred from the C++20 and C++26 runs.

#include <contracts>

#ifndef __cpp_lib_contracts
# error "__cpp_lib_contracts is not defined"
#endif
#ifndef __cpp_lib_contracts_message
# error "__cpp_lib_contracts_message is not defined"
#endif
#ifndef __cpp_lib_contracts_api
# error "__cpp_lib_contracts_api is not defined"
#endif
#ifndef __cpp_lib_contracts_implicit
# error "__cpp_lib_contracts_implicit is not defined"
#endif
#ifndef __cpp_lib_contracts_labels
# error "__cpp_lib_contracts_labels is not defined"
#endif
#ifndef __cpp_lib_contracts_report
# error "__cpp_lib_contracts_report is not defined"
#endif
#ifndef __cpp_lib_replaceable_contract_violation_handler
# error "__cpp_lib_replaceable_contract_violation_handler is not defined"
#endif

#include <cassert>
#ifndef __cpp_lib_assert_can_use_contracts
# error "__cpp_lib_assert_can_use_contracts is not defined"
#endif

// The language half, which has always worked here.
int f(int x) pre (x > 0) { return x; }

// The library half: naming the type is the whole point, since this is how a
// violation handler is declared.
void handle_contract_violation(const std::contracts::contract_violation&) { }
