/* P4405 stage 2 in C: as c-p4405-fold-prove.c with the diagnose semantic --
   a -Wcontract-unproven warning, for "always false" as for "cannot be
   proven", and nothing for a predicate that folds to true.

   Mirror: clang/test/Contracts/C/c-p4405-fold-diagnose.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose" } */

int
f (int x)
  _Pre (1)			/* { dg-bogus "cannot be proven|always false" } */
  _Pre (x > 0)			/* { dg-warning "contract assertion cannot be proven" } */
  _Post (r: sizeof (r) == 0)	/* { dg-warning "contract predicate is always false" } */
{
  _ContractAssert (0);		/* { dg-warning "contract predicate is always false" } */
  _ContractAssert (2 > 1);	/* { dg-bogus "cannot be proven|always false" } */
  return x;
}
