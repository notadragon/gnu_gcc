/* With -fcontracts-allow-assume, a C contract assertion whose semantic is
   assume is an optimizer assumption (c-assume-codegen.c), never evaluated:
   the predicate is not evaluated and the violation handler is not called.
   Here the semantic comes from the command line; c-allow-assume-config.c
   selects it from a configuration file.

   Mirror: clang/test/Contracts/C/c-allow-assume-semantic.c in the
   llvm_llvm-project fork.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume" } */

#include <contracts.h>

static int evaluations;

static int
positive (int x)
{
  ++evaluations;
  return x > 0;
}

void
handle_contract_violation (const contract_violation_t *cv)
{
  (void) cv;
  __builtin_abort ();
}

int
guarded (int x) _Pre (positive (x)) _Post (r: positive (r))
{
  _ContractAssert (positive (x));
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
