// D4298: noexcept_observe is not a terminating semantic in constant
// evaluation (a violation is a warning, matching plain observe; the
// expression remains a valid constant expression).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_observe" }
#include <contracts>

constexpr int f(int x) pre(x > 0) { return x; } // { dg-warning "contract predicate is false in constant expression" }
constexpr int bad = f(-1); // no error: noexcept_observe is not terminating

int main() { return 0; }
