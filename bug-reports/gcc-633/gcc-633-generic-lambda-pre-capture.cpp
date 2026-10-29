// GCC-633: a generic lambda's precondition naming a capture ICEs in tsubst_expr.
// g++ -std=c++26 -fcontracts -c gcc-633-generic-lambda-pre-capture.cpp
int f() { int t = 1; auto l = [t](auto u) pre(u > t) { return u + t; }; return l(2); }
