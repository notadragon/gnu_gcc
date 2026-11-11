// The last -fcontract-violation-information= wins: full then none defines
// __gcc_contracts_violation_information_none.
//
// Mirror, in the llvm_llvm-project fork:
// clang/test/Contracts/contract-violation-information-option.c; in the
// edgcpp_compiler fork, tests/tests/contracts/vinfo/option.sft.cpp.
// { dg-do compile }
// { dg-options "-std=c++26 -fcontract-violation-information=full -fcontract-violation-information=none" }

#if !defined (__gcc_contracts_violation_information_none) \
    || __gcc_contracts_violation_information_none != 1
#error "__gcc_contracts_violation_information_none is not defined to 1"
#endif
