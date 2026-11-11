// DECISIONS.md E175 (GCC-961) and E176 (GCC-964): the per-paper options
// are additive, but an explicit -fno-contracts still wins over every one of
// them, whatever the order, and each given positively is then an error.
//
// Mirror: tests/tests/contracts/p4451/option.sft.cpp in the
// edgcpp_compiler fork.

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fno-contracts -fcontracts-p4451 -fcontracts-p3850" }
// { dg-error ".-fcontracts-p4451. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p3850. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }

#ifdef __cpp_contracts
#error "contracts are off"
#endif
#ifdef __gcc_contracts_p4451
#error "P4451 is off"
#endif

int i;
