/* A handle_contract_violation with too many parameters is diagnosed
   (P4299).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

void handle_contract_violation(const contract_violation_t* cv, int extra) /* { dg-error "must have signature .void.const contract_violation_t \\*.. to be used as a contract-violation handler" } */
{
  (void) cv;
  (void) extra;
}
