/* A handle_contract_violation that is variadic is diagnosed (P4299): the
   replacement must be void (const contract_violation_t *).

   Mirror: clang/test/Contracts/C/contracts-handler-sig-variadic.c in the
   llvm_llvm-project fork; owed to EDG, recorded in its
   open-issues/README.md.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

void handle_contract_violation(const contract_violation_t* cv, ...) /* { dg-error "must have signature .void.const contract_violation_t \\*.. to be used as a contract-violation handler" } */
{
  (void) cv;
}
