/* DECISIONS.md E175-E177 (GCC-964): the contracts option rule in C,
   where -fcontracts-p3100, -fcontracts-p4298 and -fcontracts-p4405 given
   positively are errors when a final -fno-contracts-p4299 turns C contracts
   off; the order does not matter (c-contract-options-p4299-off-before.c).  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p3100 -fcontracts-p4298 -fcontracts-p4405 -fno-contracts-p4299" } */
/* { dg-error ".-fcontracts-p3100. cannot be combined with .-fno-contracts-p4299." "" { target *-*-* } 0 } */
/* { dg-error ".-fcontracts-p4298. cannot be combined with .-fno-contracts-p4299." "" { target *-*-* } 0 } */
/* { dg-error ".-fcontracts-p4405. cannot be combined with .-fno-contracts-p4299." "" { target *-*-* } 0 } */

int i;
