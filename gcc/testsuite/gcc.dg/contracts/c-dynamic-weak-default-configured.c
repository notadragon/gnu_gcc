/* P3595: the weak default of a dynamic selector returns the semantic the
   configuration specifies (D3595R1), not that semantic adjusted to the
   semantics the flags allow: the check adjusts the value it gets, so the
   weak definition is the same in a C and a C++ translation unit and
   whatever their flags.  Without -fcontracts-p4298 the configured
   noexcept_observe is evaluated as observe: the handler is called and the
   program continues, while the weak default returns noexcept_observe
   (GCC-906 returned observe).  The C++ form is
   g++.dg/contracts/cpp26/p3595-dynamic-weak-default-configured.C, whose
   configuration file this test shares.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-weak-default-configured.json" } */

#include <contracts.h>

unsigned short configured_weak_sel (void);

static int handled, seen;

void
handle_contract_violation (const contract_violation_t *cv)
{
  ++handled;
  seen = stdc_contract_violation_semantic (cv);
}

int f (int x) _Pre (x > 0) { return x; }

int
main (void)
{
  if (configured_weak_sel () != STDC_CONTRACT_NOEXCEPT_OBSERVE)
    return 1;
  f (-1);
  return handled == 1 && seen == STDC_CONTRACT_OBSERVE ? 0 : 2;
}
