// P4283: Feature-test macro for requires clauses on contracts.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4283" }

#ifndef __cpp_contracts_requires
#error "__cpp_contracts_requires is not defined"
#endif

#if __cpp_contracts_requires != 202606L
#error "__cpp_contracts_requires has wrong value"
#endif
