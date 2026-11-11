/* DECISIONS.md E175-E177 (GCC-964): the contracts option rule in C,
   where a per-paper option given positively is an error when a final
   -fno-contracts-p4299 turns C contracts off, whatever the order
   (c-contract-options-p4299-off-after.c).  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fno-contracts-p4299 -fcontracts-p3100" } */
/* { dg-error ".-fcontracts-p3100. cannot be combined with .-fno-contracts-p4299." "" { target *-*-* } 0 } */

int i;
