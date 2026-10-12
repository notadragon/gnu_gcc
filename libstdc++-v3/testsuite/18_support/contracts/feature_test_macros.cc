// { dg-options "-fcontracts-p3850" }
// { dg-do compile { target c++26 } }
// { dg-add-options no_pch }

// The five contracts feature-test macros the branch adds are exercised
// nowhere in the libstdc++ testsuite -- the whole library surface is
// covered only from g++.dg.  They are also the macros that vanish if
// bits/version.h is regenerated without the matching version.def entries,
// so a test that names each one is worth having here.

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

#include <cassert>
#ifndef __cpp_lib_assert_can_use_contracts
# error "__cpp_lib_assert_can_use_contracts is not defined"
#elif __cpp_lib_assert_can_use_contracts <= 0
# error "__cpp_lib_assert_can_use_contracts has the wrong value"
#endif
