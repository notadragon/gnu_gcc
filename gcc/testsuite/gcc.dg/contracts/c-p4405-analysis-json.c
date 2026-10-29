/* D4405 in C: configuration entries with "analysis": true select the
   semantic of the analysis evaluation in a lookup of their own (the first
   entry, which has no "analysis", does not stop the precondition's from
   being prove), and its other entries are skipped by the runtime lookups,
   which are ignore.  (The configuration errors:
   c-p4405-analysis-config-errors.c.)  The proof shadow is off,
   as in c-p4405-analysis-flag.c.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fno-contracts-prove-shadow" } */
/* { dg-additional-options "-fcontract-configuration-file=${srcdir}/gcc.dg/contracts/c-p4405-analysis-json.json" } */

int
f (int x)
  _Pre (x > 0)			/* { dg-error "contract assertion cannot be proven" } */
  _Post (r: r > 0)		/* { dg-bogus "cannot be proven" } */
{
  _ContractAssert (x != 3);	/* { dg-warning "contract assertion cannot be proven" } */
  return x;
}
