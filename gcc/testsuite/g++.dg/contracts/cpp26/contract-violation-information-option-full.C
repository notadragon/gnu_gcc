// By default (full) __gcc_contracts_violation_information_none is not
// defined, with every contracts extension enabled.
//
// Mirror, in the llvm_llvm-project fork:
// clang/test/Contracts/contract-violation-information-option.c; in the
// edgcpp_compiler fork, tests/tests/contracts/vinfo/option.sft.cpp.
// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts -fcontracts-p3850" }

#ifdef __gcc_contracts_violation_information_none
#error "__gcc_contracts_violation_information_none is defined"
#endif
