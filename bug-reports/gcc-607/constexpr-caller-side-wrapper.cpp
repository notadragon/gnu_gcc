// { dg-options "-std=c++26 -fcontracts -fcontracts-client-check=pre" }
constexpr int f (int x) pre (x > 0) { return x; }
static_assert (f (1) == 1);
