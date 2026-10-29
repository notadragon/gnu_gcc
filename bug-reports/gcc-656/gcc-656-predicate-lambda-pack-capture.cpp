// gcc-656-predicate-lambda-pack-capture.cpp                        -*-C++-*-
//
// GCC-656: a lambda in a function template's precondition that captures the
// template's parameter pack.  Expected: compiles.  Actual (this branch):
// "'xs#0' is not captured", then "sorry, unimplemented: use of
// 'type_pack_expansion' in template" at instantiation; stock: "use of
// parameter outside function body" in the template.
//
// g++ -std=c++26 -fcontracts -c gcc-656-predicate-lambda-pack-capture.cpp

template <class... T>
int g (T... xs) pre ([=] { return ((xs > 0) && ...); } ()) { return 0; }
int a = g (1, 2);
