/* P4405 stage 2 in C: as c-p4405-fold-prove.c with the diagnose semantic --
   a -Wcontract-unproven warning, for "always false" as for "cannot be
   proven", and nothing for a predicate that folds to true.  The always
   false assertion is in a function of its own: when the proof shadow
   runs, what follows it is never reached there, the postcondition's
   evaluation at the return included (DECISIONS.md E195, E196 item 1).

   Mirror: clang/test/Contracts/C/c-p4405-fold-diagnose.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose" } */

int
f (int x)
  _Pre (1)			/* { dg-bogus "(cannot be proven|always false)" } */
  _Pre (x > 0)			/* { dg-warning "contract assertion cannot be proven" } */
  _Post (r: sizeof (r) == 0)	/* { dg-warning "contract predicate is always false" } */
{
  _ContractAssert (2 > 1);	/* { dg-bogus "(cannot be proven|always false)" } */
  return x;
}

void
g (void)
{
  _ContractAssert (0);		/* { dg-warning "contract predicate is always false" } */
}
