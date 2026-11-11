// GCC-896, TU E: the same f, its assertion evaluated, and a throwing handler.
// g++ -std=c++26 -fcontracts -O2 -fcontract-evaluation-semantic=observe -c e.cc
// (=enforce behaves the same)
#include <contracts>
[[gnu::noinline]] inline int f (int x) { contract_assert (x > 0); return x; }
int (*fp_e) (int) = f;
void handle_contract_violation (const std::contracts::contract_violation &)
{ throw 42; }
