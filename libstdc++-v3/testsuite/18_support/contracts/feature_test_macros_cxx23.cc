// { dg-options "-std=gnu++23 -fcontracts-p3850" }
// { dg-do compile }
// { dg-add-options no_pch }

// The C++23 half of feature_test_macros_cxx20.cc; see that file and
// bug-reports/gcc-50/ for why the dialect is pinned and what used to fail.
// C++23 is the dialect the bug was reported against, and is covered here
// rather than inferred from the C++20 and C++26 runs.

#include <contracts>

#ifndef __cpp_lib_contracts
# error "__cpp_lib_contracts is not defined"
#elif __cpp_lib_contracts != 202502L
# error "__cpp_lib_contracts has the wrong value"
#endif
#ifndef __cpp_lib_contracts_message
# error "__cpp_lib_contracts_message is not defined"
#elif __cpp_lib_contracts_message <= 0
# error "__cpp_lib_contracts_message has the wrong value"
#endif
#ifndef __cpp_lib_contracts_api
# error "__cpp_lib_contracts_api is not defined"
#elif __cpp_lib_contracts_api <= 0
# error "__cpp_lib_contracts_api has the wrong value"
#endif
#ifndef __cpp_lib_contracts_implicit
# error "__cpp_lib_contracts_implicit is not defined"
#elif __cpp_lib_contracts_implicit <= 0
# error "__cpp_lib_contracts_implicit has the wrong value"
#endif
#ifndef __cpp_lib_contracts_labels
# error "__cpp_lib_contracts_labels is not defined"
#elif __cpp_lib_contracts_labels <= 0
# error "__cpp_lib_contracts_labels has the wrong value"
#endif
#ifndef __cpp_lib_contracts_report
# error "__cpp_lib_contracts_report is not defined"
#elif __cpp_lib_contracts_report <= 0
# error "__cpp_lib_contracts_report has the wrong value"
#endif
#ifndef __cpp_lib_replaceable_contract_violation_handler
# error "__cpp_lib_replaceable_contract_violation_handler is not defined"
#elif __cpp_lib_replaceable_contract_violation_handler != 202603L
# error "__cpp_lib_replaceable_contract_violation_handler has the wrong value"
#endif

#include <cassert>
#ifndef __cpp_lib_assert_can_use_contracts
# error "__cpp_lib_assert_can_use_contracts is not defined"
#elif __cpp_lib_assert_can_use_contracts <= 0
# error "__cpp_lib_assert_can_use_contracts has the wrong value"
#endif

// The language half, which has always worked here.
int f(int x) pre (x > 0) { return x; }

// The library half: naming the type is the whole point, since this is how a
// violation handler is declared.
void handle_contract_violation(const std::contracts::contract_violation&) { }
