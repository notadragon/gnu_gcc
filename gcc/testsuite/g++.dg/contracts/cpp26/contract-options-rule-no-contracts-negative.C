// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// Negative per-paper options are always allowed, also with -fno-contracts.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fno-contracts -fno-contracts-p3400 -fno-contracts-p3850 -fno-contracts-p4451 -fno-contracts-p3100" }

#ifdef __cpp_contracts
#error "contracts are off"
#endif

int f (int x) pre (x > 0) { return x; }	// { dg-error "expected initializer before .pre." }
