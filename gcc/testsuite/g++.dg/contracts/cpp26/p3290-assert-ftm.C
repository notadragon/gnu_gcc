// P3290: __cpp_lib_assert_can_use_contracts defined in <cassert>.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }

#include <cassert>

#ifndef __cpp_lib_assert_can_use_contracts
#error "__cpp_lib_assert_can_use_contracts not defined"
#endif
