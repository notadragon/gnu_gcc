/* D4405 in C: the analysis evaluation of a contract assertion is separate
   from its runtime evaluation, which keeps its semantic (here enforce), and
   comes first, so its proof never relies on the assertion's own check: an
   enforced check, whose violation does not return, would prove it.  A
   later assertion of the same predicate is proven by the earlier ones.  C
   knows a function's preconditions only where it is defined, so a
   precondition's analysis evaluation is in its body, ahead of its check,
   not at each call as in C++.  Pinned to -O2 (the torture options are
   skipped).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -O2 -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=diagnose" } */
/* { dg-skip-if "pinned to -O2" { *-*-* } { "-O0" "-O1" "-O3" "-Os" "-Og" "-flto" } { "" } } */

void
a (int x)
{
  _ContractAssert (x > 0);	/* { dg-warning "contract assertion cannot be proven" } */
  _ContractAssert (x > 0);	/* { dg-bogus "cannot be proven" } */
}

int
b (int x)
  _Pre (x > 0)			/* { dg-warning "contract assertion cannot be proven" } */
  _Post (r: r > 0)		/* { dg-bogus "cannot be proven" } */
{
  return x;
}

int
c (int x)
  _Post (r: r > 0)		/* { dg-warning "contract assertion cannot be proven" } */
{
  return x;
}
