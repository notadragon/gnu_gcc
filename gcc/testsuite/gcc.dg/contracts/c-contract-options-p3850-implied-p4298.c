/* DECISIONS.md E179 (1) (GCC-970): on a C input -fcontracts-p3850 implies
   -fcontracts-p4298, so a noexcept semantic is not downgraded: the handler
   sees STDC_CONTRACT_NOEXCEPT_OBSERVE (as with -fcontracts-p4298 itself,
   c-noexcept-semantic.c).  */
/* { dg-do run } */
/* { dg-options "-std=c23 -fcontracts-p3850 -fcontract-evaluation-semantic=noexcept_observe" } */

#include <contracts.h>

static int handler_called = 0;
static int a_semantic = 0;

void handle_contract_violation (const contract_violation_t *cv)
{
  handler_called++;
  a_semantic = stdc_contract_violation_semantic (cv);
}

int f (int x) _Pre (x > 0) { return x; }

int main (void)
{
  f (0);
  if (handler_called != 1)
    __builtin_abort ();
  if (a_semantic != STDC_CONTRACT_NOEXCEPT_OBSERVE)
    __builtin_abort ();
  return 0;
}
