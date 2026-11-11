/* DECISIONS.md E175-E177 (GCC-964): the contracts option rule in C,
   where the contracts options warn that they have no effect when an
   explicit -fno-contracts-p4299 turns C contracts off, whatever the order,
   the configuration options with their own warning.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-allow-assume -fcontracts-p4299 -fno-contracts-p4299 -fcontract-group-evaluation-semantic=g:observe" } */
/* { dg-warning ".-fcontracts-allow-assume. has no effect without .-fcontracts-p4299." "" { target *-*-* } 0 } */
/* { dg-warning "contract configuration options have no effect without .-fcontracts-p4299." "" { target *-*-* } 0 } */

int i;
