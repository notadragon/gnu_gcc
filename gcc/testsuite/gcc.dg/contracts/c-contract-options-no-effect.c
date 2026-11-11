/* DECISIONS.md E175-E177 (GCC-964): the contracts option rule in C,
   where the contracts options that mean nothing without C contracts warn
   that they have no effect without -fcontracts-p4299, as do the P4405
   strategies given positively; a joined option is named without its
   argument.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-allow-assume -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=ignore -fcontracts-client-check=all -fcontracts-definition-check=off -fcontracts-prove-fold -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-assume -fcontracts-prove-callee-premises -fcontracts-prove-optimize=0" } */
/* { dg-warning ".-fcontracts-allow-assume. has no effect without .-fcontracts-p4299." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontract-evaluation-semantic=. has no effect without .-fcontracts-p4299." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontract-analysis-semantic=. has no effect without .-fcontracts-p4299." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-client-check=. has no effect without .-fcontracts-p4299." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-definition-check=. has no effect without .-fcontracts-p4299." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-prove-fold. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-prove-shadow. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-prove-override. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-prove-assume. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-prove-callee-premises. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-prove-optimize=. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 } */

#ifdef __gcc_contracts_p4299
#error "-fcontracts-p4299 is off"
#endif

int i;
