// { dg-options "-std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce" }
constexpr int cf (int x) pre (x > 0) { return x; }
struct S {
  constexpr S (int x) pre (x > 0) : v (x) {}
  constexpr int mf (int x) const pre (x < v)
  { contract_assert (x != 2); return x; }
  int v;
};
constexpr auto lam = [] (int x) pre (x > 0) { return x; };
