// { dg-options "-std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore" }
constexpr int f (int x) pre (x > 0) { return x; }
