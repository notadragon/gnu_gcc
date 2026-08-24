// P4283 x constant evaluation: a requires-clause on a contract of a constexpr
// function template, evaluated at compile time.  When the constraint is
// satisfied the contract is kept and checked during constant evaluation; when it
// is not satisfied the contract is discarded and imposes nothing.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283" }

#include <concepts>

template <typename T>
constexpr T f(T x)
  pre requires(std::integral<T>) (x > 0) // { dg-error "contract predicate is false in constant expression" }
{ return x; }

// int: constraint satisfied -> precondition kept and evaluated at CE; x=-1 fails.
constexpr int y = f(-1);

// double: constraint not satisfied -> precondition discarded -> valid constant.
constexpr double z = f(1.5);
