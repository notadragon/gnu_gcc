// As template-decl-contract-lambda.C, for a function template at namespace
// scope.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

template <class T> void fa (T t) pre ([&] { return t > 0; } ());
