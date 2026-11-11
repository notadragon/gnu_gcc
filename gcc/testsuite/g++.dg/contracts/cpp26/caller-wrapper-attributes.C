// Calls through a caller-side contract wrapper keep the callee's noreturn
// (no -Wreturn-type) and nodiscard (-Wunused-result), and the
// warn_unused_result warning names the callee, not the wrapper.
//
// No Clang mirror: Clang has no caller-side checking.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -O2 -Wall" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/open-bug-caller-side.json" }

[[noreturn]] void die (int x) pre (x > 0);
int f (int x) { if (x) return x; die (x); }	// { dg-bogus "control reaches end" }
[[nodiscard]] int nd (int x) pre (x > 0);
void use1 () { nd (1); }	// { dg-warning "ignoring return value of .int nd\\(int\\)." }
__attribute__((warn_unused_result)) int wur (int x) pre (x > 0);
void use2 () { wur (1); }	// { dg-bogus "contract_wrapper" }
// { dg-warning "ignoring return value of .int wur\\(int\\)." "" { target *-*-* } .-1 }
