// GCC-896 (memory form), TU I: f with its assertion ignored, and a caller.
// g++ -std=c++26 -fcontracts -O2 -fcontract-evaluation-semantic=ignore -c pure-i.cc
int g;
[[gnu::noinline]] inline int f (int x) { contract_assert (x > 0); return x; }
int use_g (int y) { g = 1; int r = f (y); return r + g; }
