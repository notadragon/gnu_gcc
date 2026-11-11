/* P4405 in C: every evaluation with the diagnose semantic gets a
   -Wcontract-unproven warning, on by default; -Wno-contract-unproven
   silences it (c-p4405-not-evaluated.c).  The proof shadow is off: under
   the torture options it would prove the postcondition from the unproven
   precondition.

   Mirror: clang/test/Contracts/C/c-p4405-diagnose.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fno-contracts-prove-shadow -fcontract-evaluation-semantic=diagnose" } */

int
f (int x)
  _Pre (x > 0)			/* { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" } */
  _Post (r: r > 0)		/* { dg-warning "contract assertion cannot be proven" } */
{
  _ContractAssert (x != 3);	/* { dg-warning "contract assertion cannot be proven" } */
  return x;
}
