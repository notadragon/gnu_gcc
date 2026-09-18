// P3099: Verify feature-test macros are defined.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3099" }

#ifndef __cpp_contracts_message
#error "__cpp_contracts_message not defined"
#endif

static_assert(__cpp_contracts_message >= 202606L);

#include <contracts>

#ifndef __cpp_lib_contracts_message
#error "__cpp_lib_contracts_message not defined"
#endif

static_assert(__cpp_lib_contracts_message >= 202606L);
