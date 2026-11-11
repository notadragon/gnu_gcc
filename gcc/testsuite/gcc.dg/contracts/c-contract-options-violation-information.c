/* DECISIONS.md E175-E177 (GCC-964): the contracts option rule in C,
   where -fcontract-violation-information= does not warn without
   -fcontracts-p4299: <contracts.h> honours it whatever the contracts
   options.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontract-violation-information=none" } */
/* { dg-bogus "has no effect" "" { target *-*-* } 0 } */

#ifndef __gcc_contracts_violation_information_none
#error "__gcc_contracts_violation_information_none"
#endif

int i;
