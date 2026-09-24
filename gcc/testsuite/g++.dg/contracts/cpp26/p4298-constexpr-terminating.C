// D4298: noexcept_enforce is a terminating semantic in constant evaluation
// (a violation is a hard error, matching plain enforce).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_enforce" }
#include <contracts>

constexpr int f(int x) pre(x > 0) { return x; } // { dg-error "contract predicate is false in constant expression" }
constexpr int bad = f(-1);

int main() { return 0; }
