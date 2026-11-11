// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// With -fno-contracts, -fcontracts-p3400 -fno-contracts-p4451 is diagnosed
// for -fcontracts-p3400 given with -fno-contracts, not for
// -fno-contracts-p4451 taking part of P3400 away (any other error is an
// excess error).

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fno-contracts -fcontracts-p3400 -fno-contracts-p4451" }
// { dg-error ".-fcontracts-p3400. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }

int i;
