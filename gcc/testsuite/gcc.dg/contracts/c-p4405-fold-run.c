/* P4405 stage 2 in C: folding never evaluates the predicate at run time --
   a predicate true whatever its operand's side effects is not proven, and
   none of them run -- and -fcontracts-prove-assume is accepted and changes
   nothing.  diagnose is used so that the program links and runs.  The
   proof shadow is switched off, since under the torture options it would
   prove what folding does not.

   Mirror: clang/test/Contracts/C/c-p4405-fold-run.c in the
   llvm_llvm-project fork.  Clang does not fold x == x, so its mirror
   expects that one unproven.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fcontracts-prove-assume -fno-contracts-prove-shadow -fcontract-evaluation-semantic=diagnose" } */

#include <contracts.h>

static int evaluations;

static int
touch (void)
{
  ++evaluations;
  return 0;
}

void
handle_contract_violation (const contract_violation_t *cv)
{
  (void) cv;
  __builtin_abort ();
}

int
guarded (int x)
  _Pre (touch () || 1)		/* { dg-warning "contract assertion cannot be proven" } */
  _Pre (x == x)
  _Post (r: (touch (), 1))	/* { dg-warning "contract assertion cannot be proven" } */
{
  _ContractAssert (touch () && 0);	/* { dg-warning "contract assertion cannot be proven" } */
  _ContractAssert (0);		/* { dg-warning "contract predicate is always false" } */
  _ContractAssert (1);
  return x;
}

int
main (void)
{
  guarded (-1);
  if (evaluations != 0)
    __builtin_abort ();
  return 0;
}
