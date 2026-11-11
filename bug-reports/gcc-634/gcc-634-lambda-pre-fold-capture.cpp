// GCC-634: a fold over a captured pack in a lambda's precondition.
// Stock: "could not convert" then an ICE; this branch: an ICE.
// g++ -std=c++26 -fcontracts -c gcc-634-lambda-pre-fold-capture.cpp
template<class... Ts> int p(Ts... ts)
{
  auto l = [=] (int u) pre (((u > ts) && ...)) { return (u + ... + ts); };
  return l(5);
}
int x = p(1, 2);
