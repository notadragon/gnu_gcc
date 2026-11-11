// P4283: the requires-clause is part of the contract, so it must match between a
// declaration and its later definition (complements p4283-redecl.C, which
// checks declaration-vs-declaration).  A matching requires-clause is accepted; a
// different one on the definition is diagnosed.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283" }

#include <concepts>

// Same requires-clause on declaration and definition -- OK.
template <typename T> void same(T x) pre requires(std::integral<T>) (x > 0);
template <typename T> void same(T x) pre requires(std::integral<T>) (x > 0) { }

// Different requires-clause on the definition -- error.
template <typename T> void diff(T x) pre requires(std::integral<T>) (x > 0);
template <typename T> void diff(T x) pre requires(std::floating_point<T>) (x > 0) { }  // { dg-error "mismatched requires clause on contract assertion" }

void use() { same(1); }
