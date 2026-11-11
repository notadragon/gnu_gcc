// DECISIONS.md E175-E178: the contracts option rule.
// A joined contracts option warns with contracts off for any value, its
// default included (E178 (5)), as does a positive boolean option whose
// default is on.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fno-contracts -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=ignore -fcontracts-client-check=none -fcontracts-definition-check=on -fcontract-bypass-rethrowing-local-handler" }
// { dg-warning ".-fcontract-evaluation-semantic=. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontract-analysis-semantic=. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontracts-client-check=. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontracts-definition-check=. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontract-bypass-rethrowing-local-handler. has no effect without .-fcontracts." "" { target *-*-* } 0 }

int i;
