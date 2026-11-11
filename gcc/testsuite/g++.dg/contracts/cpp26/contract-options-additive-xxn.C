// DECISIONS.md E175 (GCC-961): -fcontracts-p3850, -fcontracts-p3400 and
// -fcontracts-p4451 are additive, whatever their order.  Columns of the name:
// p3850, p3400, p4451; y given, n given as -fno-, x not given.
// Expected: P3400 off, P4451 off, contracts off.
// -std=c++23, so that only the options can turn contracts on.

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fno-contracts-p4451" }

#ifdef __cpp_contracts
#error "contracts are off"
#endif
#ifdef __cpp_contracts_labels
#error "P3400 is off"
#endif
#ifdef __gcc_contracts_p4451
#error "P4451 is off"
#endif

int f (int x) pre (x > 0) { return x; }	// { dg-error "expected initializer before .pre." }
