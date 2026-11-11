/* P4405 in C (an extension, with -fcontracts-p4405): every evaluation with
   the prove semantic is an error -- a precondition, a postcondition (once,
   at its declaration, not at each return) and an assertion.  The proof
   shadow is off: under the torture options it would prove the
   postcondition from the unproven precondition.

   Mirror: clang/test/Contracts/C/c-p4405-prove.c in the llvm_llvm-project
   fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fno-contracts-prove-shadow -fcontract-evaluation-semantic=prove" } */

int
f (int x)
  _Pre (x > 0)			/* { dg-error "contract assertion cannot be proven" } */
  _Post (r: r > 0)		/* { dg-error "contract assertion cannot be proven" } */
{
  _ContractAssert (x != 3);	/* { dg-error "contract assertion cannot be proven" } */
  if (x == 4)
    return 1;
  return x;
}
