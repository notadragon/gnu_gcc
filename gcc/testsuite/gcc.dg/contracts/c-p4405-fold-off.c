/* P4405 in C: -fno-contracts-prove-fold, with the proof shadow switched off
   too, gives stage 1's behaviour back: every evaluation with prove is an
   error, "cannot be proven", even when its predicate is true or false.
   (Under the torture options the shadow would otherwise prove them.)

   Mirror: clang/test/Contracts/C/c-p4405-fold-off.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fno-contracts-prove-fold -fno-contracts-prove-shadow -fcontract-evaluation-semantic=prove" } */

void
f (void)
  _Pre (1)			/* { dg-error "contract assertion cannot be proven" } */
{
  _ContractAssert (1);		/* { dg-error "contract assertion cannot be proven" } */
  _ContractAssert (0);		/* { dg-error "contract assertion cannot be proven" } */
}
