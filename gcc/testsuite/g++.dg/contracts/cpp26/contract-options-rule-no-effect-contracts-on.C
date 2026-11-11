// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// With contracts on, the contracts options that would mean nothing with
// contracts off do not warn.

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fcontracts -fcontracts-allow-assume -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=ignore -fcontracts-client-check=all -fcontracts-definition-check=off -fcontract-checks-outlined -fcontract-disable-optimized-checks -fno-contract-bypass-rethrowing-local-handler -fcontract-comment-from-expression" }
// { dg-bogus "has no effect" "" { target *-*-* } 0 }

int f (int x) pre (x > 0) { return x; }
