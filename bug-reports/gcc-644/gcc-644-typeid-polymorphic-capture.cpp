// gcc-644-typeid-polymorphic-capture.cpp                         -*-C++-*-
//
// GCC-644: typeid of a glvalue of polymorphic class type evaluates its
// operand ([expr.typeid]), so a local named there is odr-used, and a lambda
// with no capture-default must capture it ([expr.prim.lambda.capture]/7
// captures "if the effect of any enclosing typeid expressions were
// ignored").  GCC trunk accepts; GCC 16 rejects.
//
// Expected: an error ("'x' is not captured").
// Actual:   accepted (trunk since r17-2200).
//
// g++ -std=c++20 -fsyntax-only gcc-644-typeid-polymorphic-capture.cpp

#include <typeinfo>

struct B { virtual ~B (); };
void f (B x) { auto l = [] { return &typeid (x); }; }
