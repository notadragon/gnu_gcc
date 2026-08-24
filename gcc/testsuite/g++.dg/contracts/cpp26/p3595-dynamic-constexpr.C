// P3595: dynamic is never called at compile time; "semantic" is used in CE.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-constexpr.json" }
#include <contracts>
constexpr int g(int x) pre(x > 0) { return x; } // { dg-error "contract predicate is false in constant expression" }
// In constant evaluation, entry 1 matches (constexpr:true) and uses semantic
// "enforce" (no call emitted) -> a false predicate is a compile-time error.
constexpr int bad = g(-1);
constexpr int ok  = g(1);    // no error
