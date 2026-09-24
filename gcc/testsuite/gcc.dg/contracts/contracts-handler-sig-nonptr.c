/* A handle_contract_violation whose parameter is not a pointer is
   diagnosed (D4299).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

void handle_contract_violation(int cv) /* { dg-error "must have signature" } */
{
  (void) cv;
}
