/* By default (full) __gcc_contracts_violation_information_none is not
   defined, with C contracts enabled.

   Mirror, in the llvm_llvm-project fork:
   clang/test/Contracts/contract-violation-information-option.c.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#ifdef __gcc_contracts_violation_information_none
#error "__gcc_contracts_violation_information_none is defined"
#endif
