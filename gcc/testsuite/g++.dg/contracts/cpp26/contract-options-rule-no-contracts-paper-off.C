// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// The final values decide: a per-paper option given positively and then
// negatively is off, so a final -fno-contracts is no error.

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fcontracts-p3400 -fno-contracts-p3400 -fno-contracts" }

#ifdef __cpp_contracts
#error "contracts are off"
#endif

int i;
