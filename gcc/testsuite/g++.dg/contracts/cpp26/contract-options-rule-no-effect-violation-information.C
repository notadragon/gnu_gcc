// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// -fcontract-violation-information= does not warn with contracts off:
// <contracts.h> and <cassert> honour it whatever the contracts options
// (contract-violation-information-option-none.C).

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fno-contracts -fcontract-violation-information=none" }
// { dg-bogus "has no effect" "" { target *-*-* } 0 }

#ifndef __gcc_contracts_violation_information_none
#error "__gcc_contracts_violation_information_none"
#endif

int i;
