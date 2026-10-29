// With the contract-integrated assert, assert (a, b) is ill-formed: the
// argument is not a single assignment-expression ([assertions.assert] p3).
// A parenthesized comma expression, and a template argument list containing a
// comma, are still accepted.
//
// Mirror: clang/test/Contracts/cassert-comma-expression.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3290 -Wno-unused-value" }

#define __STDC_WANT_ASSERT_USES_CONTRACTS__ 1
#include <cassert>

template<typename T, typename U> constexpr bool same = false;
template<typename T> constexpr bool same<T, T> = true;

void f (int a, int b)
{
  assert (a, b);  // { dg-message "in expansion of macro .assert." }
  assert ((a, b));
  assert ((same<int, int>));
  assert (same<int, int>);
}

// The error itself is located in the macro definition.
// { dg-error "no matching function for call to .__assert_single_argument." "" { target *-*-* } 0 }
