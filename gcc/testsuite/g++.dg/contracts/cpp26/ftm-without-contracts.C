// With -fno-contracts after -fcontracts-p3850, no per-paper contracts
// feature-test macro (nor GCC vendor macro) is defined, as __cpp_contracts
// is not.
//
// Mirror: clang/test/Contracts/ftm-without-contracts.cpp in the
// llvm_llvm-project fork.
// { dg-do preprocess { target c++26 } }
// { dg-additional-options "-fcontracts-p3850 -fcontracts-allow-assume -fno-contracts" }

#if defined(__cpp_contracts)
#error "__cpp_contracts defined with contracts off"
#endif
#if defined(__cpp_contracts_labels) || defined(__cpp_contracts_message) \
    || defined(__cpp_contracts_requires) || defined(__cpp_contracts_report) \
    || defined(__cpp_contracts_postcondition_captures) \
    || defined(__cpp_contracts_nonthrowing_semantics)
#error "a per-paper contracts feature-test macro is defined with contracts off"
#endif
#if defined(__gcc_contracts_p3100) || defined(__gcc_contracts_p3290) \
    || defined(__gcc_contracts_allow_assume)
#error "a GCC contracts vendor macro is defined with contracts off"
#endif
