// P3100: gate macro is defined when -fcontracts-p3100 is active.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }

#ifndef __gcc_contracts_p3100
#error "__gcc_contracts_p3100 not defined"
#endif

#if __gcc_contracts_p3100 != 202606L
#error "__gcc_contracts_p3100 has wrong value"
#endif
