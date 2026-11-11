/* DECISIONS.md E175-E177 (GCC-964): the contracts option rule in C,
   where a P4405 strategy given positively with C contracts on but P4405
   off warns that it has no effect; one given negatively does not.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p4299 -fcontracts-prove-fold -fno-contracts-prove-shadow" } */
/* { dg-warning ".-fcontracts-prove-fold. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 } */

int f (int x) _Pre (x > 0) { return x; }
