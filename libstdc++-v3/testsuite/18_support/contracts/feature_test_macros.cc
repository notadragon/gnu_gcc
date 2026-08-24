// { dg-options "-fcontracts-p3850" }
// { dg-do compile { target c++26 } }

// The five contracts feature-test macros the branch adds are exercised
// nowhere in the libstdc++ testsuite -- the whole library surface is
// covered only from g++.dg.  They are also the macros that vanish if
// bits/version.h is regenerated without the matching version.def entries,
// so a test that names each one is worth having here.

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
#ifndef __cpp_lib_contracts_labels
# error "__cpp_lib_contracts_labels is not defined"
#endif
#ifndef __cpp_lib_contracts_report
# error "__cpp_lib_contracts_report is not defined"
#endif

#include <cassert>
#ifndef __cpp_lib_assert_can_use_contracts
# error "__cpp_lib_assert_can_use_contracts is not defined"
#endif
