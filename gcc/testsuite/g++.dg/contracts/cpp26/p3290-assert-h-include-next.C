// libstdc++'s <assert.h> wrapper follows the other "C" compatibility
// headers: included from inside another C header's #include_next (marked by
// _GLIBCXX_INCLUDE_NEXT_C_HEADERS, as <cassert> and <cstdlib> do it) it is
// the platform's <assert.h> and nothing more, and otherwise it provides
// <bits/c++config.h> as it did when it was <cassert>.  (GCC-184.)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3290 -D__STDC_WANT_ASSERT_USES_CONTRACTS__" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#define _GLIBCXX_INCLUDE_NEXT_C_HEADERS
#include <assert.h>
#undef _GLIBCXX_INCLUDE_NEXT_C_HEADERS

// The platform's assert, not the contract one.
#ifdef __cpp_lib_assert_can_use_contracts
#error "the P3290 integration was reached through a nested include"
#endif
#ifdef _GLIBCXX_RELEASE
#error "<bits/c++config.h> was reached through a nested include"
#endif

#include <assert.h>

#ifndef _GLIBCXX_RELEASE
#error "<assert.h> does not provide <bits/c++config.h>"
#endif
#ifndef __cpp_lib_assert_can_use_contracts
#error "<assert.h> does not reach the P3290 integration"
#endif
