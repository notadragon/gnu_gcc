// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// The contracts options that mean nothing with contracts off warn that they
// have no effect when an explicit -fno-contracts turns contracts off, the
// configuration options with their own warning, whatever the order.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-allow-assume -fno-contracts -fcontract-evaluation-semantic=observe -fcontract-group-evaluation-semantic=g:observe" }
// { dg-warning ".-fcontracts-allow-assume. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontract-evaluation-semantic=. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning "contract configuration options have no effect without .-fcontracts." "" { target *-*-* } 0 }

int i;
