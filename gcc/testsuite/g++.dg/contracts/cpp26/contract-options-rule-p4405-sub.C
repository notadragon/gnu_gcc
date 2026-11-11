// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// The P4405 sub-options given positively without -fcontracts-p4405 warn
// that they have no effect; a joined option, named without its argument,
// warns for any value.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-prove-fold -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-assume -fcontracts-prove-callee-premises -fcontracts-prove-optimize=0 -fcontract-analysis-skip-dead-calls" }
// { dg-warning ".-fcontracts-prove-fold. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 }
// { dg-warning ".-fcontracts-prove-shadow. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 }
// { dg-warning ".-fcontracts-prove-override. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 }
// { dg-warning ".-fcontracts-prove-assume. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 }
// { dg-warning ".-fcontracts-prove-callee-premises. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 }
// { dg-warning ".-fcontracts-prove-optimize=. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 }
// { dg-warning ".-fcontract-analysis-skip-dead-calls. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 }

int f (int x) pre (x > 0) { return x; }
