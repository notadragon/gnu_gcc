/* With -fcontracts-allow-assume, a dynamically selected contract whose
   configuration names assume as the default semantic gets a weak default
   selector that returns assume, which takes the ignore path: the predicate
   is not evaluated and nothing is reported (GCC-690 was an enforced
   violation here).  The C++ form is
   g++.dg/contracts/cpp26/p3595-dynamic-weak-default-assume.C, whose
   configuration file this test shares.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontracts-allow-assume -fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-weak-default-assume.json" } */

#include <contracts.h>

static int violations, evaluated;

void
handle_contract_violation (const contract_violation_t *cv)
{
  (void) cv;
  ++violations;
}

static int chk (int x) { ++evaluated; return x > 0; }

int f (int x) _Pre (chk (x)) { return x; }

int
main (void)
{
  f (-1);
  return violations + evaluated;
}
