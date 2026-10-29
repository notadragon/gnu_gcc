// P3100: vendor macro is not defined without -fcontracts-allow-assume.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }

#ifdef __gcc_contracts_allow_assume
#error "__gcc_contracts_allow_assume should not be defined without -fcontracts-allow-assume"
#endif
