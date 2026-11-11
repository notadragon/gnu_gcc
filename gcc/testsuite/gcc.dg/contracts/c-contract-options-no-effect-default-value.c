/* DECISIONS.md E175-E178: the contracts option rule in C, where a
   joined contracts option warns without C contracts for any value, its
   default included (E178 (5)).  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontract-evaluation-semantic=enforce -fcontracts-definition-check=on" } */
/* { dg-warning ".-fcontract-evaluation-semantic=. has no effect without .-fcontracts-p4299." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-definition-check=. has no effect without .-fcontracts-p4299." "" { target *-*-* } 0 } */

int i;
