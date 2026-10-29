// -fcontracts-p4299 in C++ predefines __gcc_contracts_p4299 too, since
// _Pre, _Post and _ContractAssert are keywords there as well; without it,
// or without -fcontracts, the macro is not defined
// (p4299-feature-macro-without.C).  Its value is the date of the
// implemented revision, so only check that it is positive.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4299" }

#if !defined (__gcc_contracts_p4299) || __gcc_contracts_p4299 <= 0
#error "__gcc_contracts_p4299"
#endif
