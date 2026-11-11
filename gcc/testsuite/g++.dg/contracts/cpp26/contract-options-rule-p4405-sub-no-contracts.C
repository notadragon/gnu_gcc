// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// A P4405 sub-option given positively warns when contracts end up off
// (P4405 then is off too).

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fcontracts-prove-shadow" }
// { dg-warning ".-fcontracts-prove-shadow. has no effect without .-fcontracts-p4405." "" { target *-*-* } 0 }

int i;
