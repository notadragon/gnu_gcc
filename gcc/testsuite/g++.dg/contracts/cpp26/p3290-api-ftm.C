// P3290: Feature-test macro is defined when flag is active.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }

#ifndef __gcc_contracts_p3290
#error "__gcc_contracts_p3290 not defined"
#endif

#if __gcc_contracts_p3290 != 202606L
#error "__gcc_contracts_p3290 has wrong value"
#endif
