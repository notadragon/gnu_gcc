// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// A P4405 sub-option given positively warns when P4405 ends up off, here
// given after -fcontracts-p3850 -fno-contracts-p4405
// (contract-options-rule-p4405-sub-umbrella-off.C gives it before).

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3850 -fno-contracts-p4405 -fcontracts-prove-fold" }
// { dg-warning ".-fcontracts-prove-fold. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 }

int f (int x) pre (x > 0) { return x; }
