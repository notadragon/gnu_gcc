/* P4405 stage 2 in C, -fcontracts-prove-fold (implied by -fcontracts-p4405):
   an evaluation with the prove semantic whose predicate folds to true is
   proven and not diagnosed, whether or not it is a constant expression; one
   that folds to false is "always false"; anything else still "cannot be
   proven".  A postcondition is diagnosed once, at its declaration.  The
   proof shadow is off: under the torture options it would prove the
   unproven postcondition from the unproven precondition.

   Mirror: clang/test/Contracts/C/c-p4405-fold-prove.c in the
   llvm_llvm-project fork; the folds of non-constant expressions are
   GCC-specific (x == x, u >= 0u, r == r and x != x: Clang's mirror
   expects them unproven).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fno-contracts-prove-shadow -fcontract-evaluation-semantic=prove" } */

int
f (int x, unsigned u)
  _Pre (1)			/* { dg-bogus "cannot be proven|always false" } */
  _Pre (sizeof (int) >= 1)	/* { dg-bogus "cannot be proven|always false" } */
  _Pre (x == x)			/* { dg-bogus "cannot be proven|always false" } */
  _Pre (u >= 0u)		/* { dg-bogus "cannot be proven|always false" } */
  _Pre (x > 0)			/* { dg-error "contract assertion cannot be proven" } */
  _Post (r: r == r)		/* { dg-bogus "cannot be proven|always false" } */
  _Post (r: r > 0)		/* { dg-error "contract assertion cannot be proven" } */
{
  _ContractAssert (1 + 1 == 2);	/* { dg-bogus "cannot be proven|always false" } */
  _ContractAssert (0);		/* { dg-error "contract predicate is always false" } */
  _ContractAssert (x != x);	/* { dg-error "contract predicate is always false" } */
  _ContractAssert (x != 3);	/* { dg-error "contract assertion cannot be proven" } */
  if (x == 4)
    return 1;
  return x;
}
