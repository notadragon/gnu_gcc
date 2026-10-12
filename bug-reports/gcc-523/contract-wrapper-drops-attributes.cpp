// Calls through a caller-side contract wrapper lose the callee's
// noreturn and nodiscard, and diagnostics name the wrapper.
//
//   g++ -std=c++26 -fcontracts -fcontracts-client-check=all -Wall -c contract-wrapper-drops-attributes.cpp
//
// Expected (what the same command without -fcontracts-client-check=all
// gives): -Wunused-result on lines 13 and 15, naming nd and wur, and no
// -Wreturn-type on line 11.  Seen: -Wreturn-type on line 11, nothing on
// line 13, and line 15's warning names `wur.contract_wrapper'.
[[noreturn]] void die (int x) pre (x > 0);
int f (int x) { if (x) return x; die (x); }
[[nodiscard]] int nd (int x) pre (x > 0);
void use1 () { nd (1); }
__attribute__((warn_unused_result)) int wur (int x) pre (x > 0);
void use2 () { wur (1); }
