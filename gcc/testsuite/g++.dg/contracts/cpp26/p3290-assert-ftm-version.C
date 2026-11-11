// P3290: __cpp_lib_assert_can_use_contracts is defined when <version> is
// included (paper requires it in <version>, <assert.h>, and <cassert>).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }

#include <version>

#ifndef __cpp_lib_assert_can_use_contracts
#error "__cpp_lib_assert_can_use_contracts not defined via <version>"
#endif
