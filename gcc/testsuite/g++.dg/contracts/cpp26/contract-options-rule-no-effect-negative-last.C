// DECISIONS.md E175-E178: the contracts option rule.
// A contracts option given positively and then negatively ends up in its
// negative form, which does not warn (E178 (5)).

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fcontracts-allow-assume -fno-contracts-allow-assume -fcontract-checks-outlined -fno-contract-checks-outlined" }
// { dg-bogus "has no effect" "" { target *-*-* } 0 }

int i;
