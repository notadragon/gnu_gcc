/* A dynamic selector that returns a value that is not a semantic makes an
   invalid selection, reported as an enforced violation without evaluating
   the predicate: the handler runs, then the program terminates.  The
   selector has C++ linkage, so it is defined here under its mangled name.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-configuration-file=${srcdir}/gcc.dg/contracts/c-dynamic-selection-invalid.json" } */
/* { dg-shouldfail "enforced violation" } */

#include <contracts.h>
#include <stdio.h>

unsigned short bad_sel (void) __asm__ ("_Z12cdyn_bad_selv");
unsigned short bad_sel (void) { return 99; }
void handle_contract_violation (const contract_violation_t *cv)
{
  printf ("handled\n");
  fflush (stdout);
}

static int evaluated = 0;
static int check (void) { ++evaluated; return 1; }
int f (void) _Pre (check ()) { return evaluated; }

int main (void)
{
  f ();
  printf ("survived\n");
  return 0;
}

/* { dg-output "handled" } */
