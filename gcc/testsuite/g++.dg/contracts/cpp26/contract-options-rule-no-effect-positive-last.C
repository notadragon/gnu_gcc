// DECISIONS.md E175-E178: the contracts option rule.
// A contracts option given negatively and then positively ends up in its
// positive form, which warns with contracts off (E178 (5)).

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fno-contracts-allow-assume -fcontracts-allow-assume -fno-contract-checks-outlined -fcontract-checks-outlined" }
// { dg-warning ".-fcontracts-allow-assume. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontract-checks-outlined. has no effect without .-fcontracts." "" { target *-*-* } 0 }

int i;
