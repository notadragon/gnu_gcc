/* A dynamic selector with provideweak that the program does not define: the
   weak default, which returns the entry's static semantic (observe), decides.
   The C++-linkage name is mangled as C++ would mangle it (the program
   links only if the weak default carries that name).  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-configuration-file=${srcdir}/gcc.dg/contracts/c-dynamic-selection-weak.json" } */

#include <contracts.h>

static int handled = 0;
void handle_contract_violation (const contract_violation_t *cv) { ++handled; }

int f (int x) _Pre (x > 0) { return x; }

int main (void)
{
  f (-1);
  return handled == 1 ? 0 : 1;
}
