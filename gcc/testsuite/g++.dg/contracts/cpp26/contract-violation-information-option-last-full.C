// The last -fcontract-violation-information= wins: none then full leaves
// __gcc_contracts_violation_information_none undefined.
//
// Mirror, in the llvm_llvm-project fork:
// clang/test/Contracts/contract-violation-information-option.c; in the
// edgcpp_compiler fork, tests/tests/contracts/vinfo/option.sft.cpp.
// { dg-do compile }
// { dg-options "-std=c++26 -fcontract-violation-information=none -fcontract-violation-information=full" }

#ifdef __gcc_contracts_violation_information_none
#error "__gcc_contracts_violation_information_none is defined"
#endif
