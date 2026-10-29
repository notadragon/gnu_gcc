// GCC-896 (memory form), TU E: f observed; the handler writes g.
// g++ -std=c++26 -fcontracts -O2 -fcontract-evaluation-semantic=observe -c pure-e.cc
#include <contracts>
extern int g; int calls;
[[gnu::noinline]] inline int f (int x) { contract_assert (x > 0); return x; }
int (*fp_e) (int) = f;
void handle_contract_violation (const std::contracts::contract_violation &)
{ ++calls; g = 5; }
