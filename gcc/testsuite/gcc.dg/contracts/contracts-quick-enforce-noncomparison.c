/* Test _ContractAssert with a non-comparison predicate under
   quick_enforce.

   The truth-value-before-fold ordering applies at four condition-building
   sites, and only three of them are reachable under observe or
   enforce.  No other test reaches the quick_enforce site with a
   non-comparison predicate -- contracts-assert-quick-enforce.c uses
   `x > 0`, which is exactly the shape the ordering does not matter for --
   so this file is that site's only coverage.  */

/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=quick_enforce" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */
/* { dg-shouldfail "" } */

static int check (int x) { return x; }

int
main (void)
{
  /* Satisfied: a bare call, not a comparison.  */
  _ContractAssert (check (1));
  /* Violated: must trap rather than ICE at compile time.  */
  _ContractAssert (check (0));
  return 0;
}
