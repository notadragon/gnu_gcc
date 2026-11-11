// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// A per-paper option given positively is an error when a final
// -fno-contracts turns contracts off; the order does not matter
// (contract-options-rule-no-contracts-before.C).

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fcontracts-p3400 -fno-contracts" }
// { dg-error ".-fcontracts-p3400. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }

int i;
