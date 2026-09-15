/* A handle_contract_violation whose parameter points to a non-const
   contract_violation_t is diagnosed (D4299).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

void handle_contract_violation(contract_violation_t* cv) /* { dg-error "must have signature" } */
{
  (void) cv;
}
