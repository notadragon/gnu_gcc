// P3290: API declarations are available with flag.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

#ifndef __cpp_lib_contracts_api
#error "__cpp_lib_contracts_api not defined"
#endif

static_assert(static_cast<int>(std::contracts::assertion_kind::manual) == 4);
static_assert(static_cast<int>(std::contracts::assertion_kind::cassert) == 5);
static_assert(static_cast<int>(std::contracts::detection_mode::unspecified) == 0);

// Verify declarations exist (address-of)
void (*p1)(const char*, const std::source_location&)
    = &std::contracts::handle_observed_contract_violation;
