/* D4405 in C: -fcontract-analysis-semantic= selects the semantic of the
   analysis evaluation of every contract assertion, separately from the
   runtime semantic (here ignore): diagnose warns for each one that cannot
   be proven.  The proof shadow is off: under the torture options it would
   prove the postcondition from the unproven precondition.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fno-contracts-prove-shadow -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=diagnose" } */

int
f (int x)
  _Pre (x > 0)			/* { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" } */
  _Post (r: r > 0)		/* { dg-warning "contract assertion cannot be proven" } */
{
  _ContractAssert (x != 3);	/* { dg-warning "contract assertion cannot be proven" } */
  return x;
}
