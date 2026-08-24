// D4301: Verify the compiler and library feature-test macros for
// contract_violation::report() are defined and have the expected value.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p4301" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#ifndef __cpp_contracts_report
#error "__cpp_contracts_report not defined"
#endif

static_assert(__cpp_contracts_report == 202607L);

#include <contracts>

#ifndef __cpp_lib_contracts_report
#error "__cpp_lib_contracts_report not defined"
#endif

static_assert(__cpp_lib_contracts_report == 202607L);
