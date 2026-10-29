/* The replacement handler must be void (const contract_violation_t *)
   (P4299); a "const volatile contract_violation_t *" parameter is rejected.

   Mirror: clang/test/Contracts/c-violation-handler-cv-param.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

void handle_contract_violation (const volatile contract_violation_t *cv) /* { dg-error "must have signature .void.const contract_violation_t \\*.. to be used as a contract-violation handler" } */
{
  (void) cv;
}
