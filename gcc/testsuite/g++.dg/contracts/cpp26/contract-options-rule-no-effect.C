// DECISIONS.md E175-E178 (GCC-964): the contracts option rule.
// The contracts options that mean nothing with contracts off warn that they
// have no effect: here contracts are off because the dialect does not turn
// them on and no option does.  A joined option is named without its
// argument.  The negative forms do not warn
// (contract-options-rule-no-effect-negative.C).

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fcontracts-allow-assume -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=ignore -fcontracts-client-check=all -fcontracts-definition-check=off -fcontract-checks-outlined -fcontract-disable-optimized-checks -fcontract-bypass-rethrowing-local-handler -fcontract-comment-from-expression" }
// { dg-warning ".-fcontracts-allow-assume. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontract-evaluation-semantic=. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontract-analysis-semantic=. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontracts-client-check=. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontracts-definition-check=. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontract-checks-outlined. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontract-disable-optimized-checks. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontract-bypass-rethrowing-local-handler. has no effect without .-fcontracts." "" { target *-*-* } 0 }
// { dg-warning ".-fcontract-comment-from-expression. has no effect without .-fcontracts." "" { target *-*-* } 0 }

#ifdef __cpp_contracts
#error "contracts are off"
#endif

int i;
