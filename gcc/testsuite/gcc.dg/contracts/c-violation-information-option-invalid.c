/* -fcontract-violation-information= accepts only full and none.

   Mirror, in the llvm_llvm-project fork:
   clang/test/Contracts/contract-violation-information-option.c.  */
/* { dg-do compile } */
/* { dg-options "-fcontract-violation-information=some" } */
/* { dg-error "the contract violation information must be 'full' or 'none', not 'some'" "" { target *-*-* } 0 } */
/* { dg-message "valid arguments to '-fcontract-violation-information=' are: full none" "" { target *-*-* } 0 } */
