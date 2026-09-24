/* Test that handle_contract_violation with wrong signature is diagnosed.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

int handle_contract_violation(const contract_violation_t* cv) /* { dg-error "must have signature" } */
{
  return 0;
}
