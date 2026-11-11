// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// The last of -fcontracts and -fno-contracts decides: a -fno-contracts
// followed by -fcontracts leaves contracts on, and the per-paper option
// given between them is no error.

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fno-contracts -fcontracts-p3400 -fcontracts" }

#ifndef __cpp_contracts_labels
#error "P3400 is on"
#endif

int f (int x) pre (x > 0) { return x; }
