// DECISIONS.md E175 (GCC-961): -fcontracts-p3850, -fcontracts-p3400 and
// -fcontracts-p4451 are additive, whatever their order.  Columns of the name:
// p3850, p3400, p4451; y given, n given as -fno-, x not given.
// Expected: P3400 off, P4451 on, contracts on.
// -std=c++23, so that only the options can turn contracts on.

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fcontracts-p4451 -fno-contracts-p3850" }

#ifndef __cpp_contracts
#error "contracts are on"
#endif
#ifdef __cpp_contracts_labels
#error "P3400 is off"
#endif
#ifndef __gcc_contracts_p4451
#error "P4451 is on"
#endif
#if !(__cpp_contracts > 202502L)
#error "P4451 raises __cpp_contracts"
#endif

int f (int x) pre (x > 0) { return x; }
