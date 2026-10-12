// P3100: vendor macro is defined when -fcontracts-allow-assume is active.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-allow-assume" }

#ifndef __gcc_contracts_allow_assume
#error "__gcc_contracts_allow_assume not defined"
#endif
