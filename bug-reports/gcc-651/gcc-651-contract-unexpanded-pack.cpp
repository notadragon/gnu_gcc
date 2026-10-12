// gcc-651-contract-unexpanded-pack.cpp                            -*-C++-*-
//
// GCC-651: a function parameter pack named unexpanded in a contract
// predicate is not diagnosed in the template.  Expected: "parameter packs
// not expanded with '...'" on lines A, B and C.  Actual: nothing; the
// instantiation of A reports "sorry, unimplemented: use of
// 'type_pack_expansion' in template", and C compiles.
//
// g++ -std=c++26 -fcontracts -c gcc-651-contract-unexpanded-pack.cpp

template <class... T> int f1 (T... xs) pre (xs > 0) { return 0; }   // A
template <class... T> int f2 (T... xs) pre (xs > 0);                 // B
template <class... T> int f3 (T... xs) { contract_assert (xs > 0); return 0; }  // C
int a = f1 (1, 2);
