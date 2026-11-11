/* -fcontract-violation-information=none predefines
   __gcc_contracts_violation_information_none (to 1) in C with no other
   contracts option.

   Mirror, in the llvm_llvm-project fork:
   clang/test/Contracts/contract-violation-information-option.c.  */
/* { dg-do compile } */
/* { dg-options "-fcontract-violation-information=none" } */

#if !defined (__gcc_contracts_violation_information_none) \
    || __gcc_contracts_violation_information_none != 1
#error "__gcc_contracts_violation_information_none is not defined to 1"
#endif
#ifdef __gcc_contracts_p4299
#error "C contracts are enabled"
#endif
