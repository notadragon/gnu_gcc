// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// No warning for a P4405 sub-option given negatively with contracts off.

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fno-contracts-prove-fold -fno-contract-analysis-skip-dead-calls" }
// { dg-bogus "has no effect" "" { target *-*-* } 0 }

int i;
