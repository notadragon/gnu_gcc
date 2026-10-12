// P3100: the "unevaluated operand" suppression that keeps implicit assertions
// out of the noexcept operator must be scoped to runtime instrumentation only.
// A constant expression nested within an unevaluated operand is still
// constant-evaluated, so a contract assertion it contains must still be
// checked.  Here f(-1) is a template argument (a constant expression) inside a
// noexcept(...) operand; its precondition x > 0 must be evaluated and fail.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

constexpr int f (int x) pre (x > 0) { return 0; } // { dg-error "contract predicate is false in constant expression" }

template <int N> bool b = true;

static_assert (noexcept (b<f (-1)>)); // { dg-error "template argument" }
