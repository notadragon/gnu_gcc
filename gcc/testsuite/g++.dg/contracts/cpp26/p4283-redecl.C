// P4283: Redeclaration matching for requires clauses.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283" }

#include <concepts>

// Matching requires clauses: OK.
template <typename T>
void f(T x)
  pre requires(std::integral<T>) (x > 0);

template <typename T>
void f(T x)
  pre requires(std::integral<T>) (x > 0);

// Mismatched requires clauses: error.
template <typename T>
void g(T x)
  pre requires(std::integral<T>) (x > 0);

template <typename T>
void g(T x)
  pre requires(std::floating_point<T>) (x > 0);  // { dg-error "mismatched" }
