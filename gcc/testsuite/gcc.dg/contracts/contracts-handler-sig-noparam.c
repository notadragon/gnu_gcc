/* A handle_contract_violation with no parameter is diagnosed (D4299).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

void handle_contract_violation(void) /* { dg-error "must have signature" } */
{
}
