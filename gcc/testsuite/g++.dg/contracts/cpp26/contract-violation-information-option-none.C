// -fcontract-violation-information=none predefines
// __gcc_contracts_violation_information_none (to 1) with no other contracts
// option and in a dialect without contracts.
//
// Mirror, in the llvm_llvm-project fork:
// clang/test/Contracts/contract-violation-information-option.c; in the
// edgcpp_compiler fork, tests/tests/contracts/vinfo/option.sft.cpp.
// { dg-do compile }
// { dg-options "-std=c++17 -fcontract-violation-information=none" }

#if !defined (__gcc_contracts_violation_information_none) \
    || __gcc_contracts_violation_information_none != 1
#error "__gcc_contracts_violation_information_none is not defined to 1"
#endif
#ifdef __cpp_contracts
#error "contracts are enabled"
#endif
