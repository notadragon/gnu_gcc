// DECISIONS.md E175 (GCC-961): -fcontracts-p3850, -fcontracts-p3400 and
// -fcontracts-p4451 are additive, whatever their order.  Columns of the name:
// p3850, p3400, p4451; y given, n given as -fno-, x not given.
// Expected: an error, since P3400 is on and -fno-contracts-p4451 would
// take away part of it.
// -std=c++23, so that only the options can turn contracts on.

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fcontracts-p3400 -fno-contracts-p4451" }
// { dg-error ".-fno-contracts-p4451. cannot be combined with .-fcontracts-p3400., which subsumes P4451" "" { target *-*-* } 0 }

int i;
