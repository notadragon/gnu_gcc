// P3290: Gate macro not defined without the flag.
// { dg-do compile { target c++26 } }

#ifdef __gcc_contracts_p3290
#error "__gcc_contracts_p3290 should not be defined without -fcontracts-p3290"
#endif
