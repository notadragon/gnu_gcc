/* A handle_contract_violation taking a pointer to something other than
   contract_violation_t is diagnosed (P4299).

   Mirror: clang/test/Contracts/C/contracts-handler-sig-pointee.c in the
   llvm_llvm-project fork; owed to EDG, recorded in its
   open-issues/README.md.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

void handle_contract_violation(const int* cv) /* { dg-error "must have signature .void.const contract_violation_t \\*.. to be used as a contract-violation handler" } */
{
  (void) cv;
}
