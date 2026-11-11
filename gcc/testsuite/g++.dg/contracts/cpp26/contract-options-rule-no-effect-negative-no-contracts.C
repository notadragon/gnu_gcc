// DECISIONS.md E175-E178: the contracts option rule.
// The negative forms of the contracts options do not warn when an explicit
// -fno-contracts turns contracts off either, whether given before or after
// it (E178 (5)).

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fno-contracts-allow-assume -fno-contract-checks-outlined -fno-contracts -fno-contract-disable-optimized-checks -fno-contract-bypass-rethrowing-local-handler -fno-contract-comment-from-expression" }
// { dg-bogus "has no effect" "" { target *-*-* } 0 }

#ifdef __cpp_contracts
#error "contracts are off"
#endif

int i;
