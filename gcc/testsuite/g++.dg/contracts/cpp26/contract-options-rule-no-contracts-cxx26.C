// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// In C++26, where the standard turns contracts on, an explicit
// -fno-contracts still turns them off, and a per-paper option given
// positively with it is an error.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fno-contracts -fcontracts-p3098" }
// { dg-error ".-fcontracts-p3098. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }

int i;
