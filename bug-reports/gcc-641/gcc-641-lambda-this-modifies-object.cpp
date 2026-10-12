// gcc-641-lambda-this-modifies-object.cpp                        -*-C++-*-
//
// GCC-641: in a contract predicate `this` is a pointer to const
// ([expr.prim.this]), also where a lambda in the predicate captures it, so
// the lambda cannot modify the object through it.  GCC accepts both lines.
//
// Expected: two errors (Clang: "cannot assign to non-static data member
// because it is considered 'const' inside of a contract").
// Actual:   accepted.
//
// g++ -std=c++26 -fcontracts -fsyntax-only gcc-641-lambda-this-modifies-object.cpp

struct S {
  int m;
  void f () pre ([this] { return ++m > 0; } ());
  void g () { contract_assert ([&] { return ++m > 0; } ()); }
};
