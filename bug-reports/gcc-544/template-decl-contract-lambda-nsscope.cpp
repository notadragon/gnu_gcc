// g++ -std=c++26 -fcontracts -fsyntax-only template-decl-contract-lambda-nsscope.cpp
//
// A lambda in the precondition of a namespace-scope function template
// declaration.  Valid code.  GCC 16.2 and trunk reject it ("use of parameter
// outside function body"): the lambda's reference to the parameter t is
// wrongly treated as a use outside a function body.

template <class T> void fa (T t) pre ([&] { return t > 0; } ());
