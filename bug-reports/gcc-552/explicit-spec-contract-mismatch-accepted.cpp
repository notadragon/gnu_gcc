// g++ -std=c++26 -fcontracts -fsyntax-only explicit-spec-contract-mismatch-accepted.cpp
//
// The two declarations of the explicit specialization ft<int> have
// different preconditions, which is ill-formed ([dcl.contract.func]: the
// function-contract-specifier-seqs of two declarations shall be the same).
// GCC 16.2 and trunk accept it.

template <class T> void ft (T x);
template <> void ft<int> (int x) pre (x > 5);
template <> void ft<int> (int x) pre (x > 6) {}
