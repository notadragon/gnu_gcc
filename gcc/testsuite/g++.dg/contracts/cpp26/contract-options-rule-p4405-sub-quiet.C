// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// No warning for a P4405 sub-option given negatively without P4405, nor
// for one given positively before -fcontracts-p4405.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fno-contracts-prove-fold -fno-contract-analysis-skip-dead-calls -fcontracts-prove-shadow -fcontracts-p4405" }
// { dg-bogus "has no effect" "" { target *-*-* } 0 }

int f (int x) pre (x > 0) { return x; }
