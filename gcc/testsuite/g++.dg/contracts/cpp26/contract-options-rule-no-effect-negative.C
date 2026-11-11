// DECISIONS.md E175-E178: the contracts option rule.
// The negative forms of the contracts options that mean nothing with
// contracts off ask for nothing, so they do not warn (E178 (5)).

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fno-contracts-allow-assume -fno-contract-checks-outlined -fno-contract-disable-optimized-checks -fno-contract-bypass-rethrowing-local-handler -fno-contract-comment-from-expression" }
// { dg-bogus "has no effect" "" { target *-*-* } 0 }

#ifdef __cpp_contracts
#error "contracts are off"
#endif

int i;
