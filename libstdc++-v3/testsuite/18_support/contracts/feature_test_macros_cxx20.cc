// { dg-options "-std=gnu++20 -fcontracts-p3850" }
// { dg-do compile }
// { dg-add-options no_pch }

// -fcontracts enables contract assertions below C++26, and the library half
// has to follow it there: every contracts feature-test macro used to carry
// cxxmin = 26 in version.def, so all of them were dead at -std=c++20 even
// with contracts on, and <contracts> expanded to nothing but its include
// guard.  That left no way to declare a violation handler, whose parameter
// names a type the header defines.  See bug-reports/gcc-50/.
//
// C++20 is the floor: the interface below names std::source_location, whose
// own feature-test macro is cxxmin = 20.
//
// The -std= in dg-options pins this to one run; without it the harness would
// pick the dialect from the target selector and defeat the point of the test.

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
