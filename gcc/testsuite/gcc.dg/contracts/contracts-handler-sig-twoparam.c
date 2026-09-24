/* A handle_contract_violation with too many parameters is diagnosed
   (D4299).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

void handle_contract_violation(const contract_violation_t* cv, int extra) /* { dg-error "must have signature" } */
{
  (void) cv;
  (void) extra;
}
