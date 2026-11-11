/* A handle_contract_violation returning int is diagnosed (P4299).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

int handle_contract_violation(const contract_violation_t* cv) /* { dg-error "must have signature .void.const contract_violation_t \\*.. to be used as a contract-violation handler" } */
{
  return 0;
}
