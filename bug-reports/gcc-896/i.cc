// GCC-896, TU I: the inline f with its assertion ignored, and two callers.
// g++ -std=c++26 -fcontracts -O2 -fcontract-evaluation-semantic=ignore -c i.cc
struct D { ~D (); };
[[gnu::noinline]] inline int f (int x) { contract_assert (x > 0); return x; }
int use_catch (int y) { try { return f (y); } catch (...) { return -1; } }
int use_dtor (int y) { D d; return f (y); }
