/* Test _ContractAssert with a non-comparison predicate under
   quick_enforce.

   Coverage gap: the fold/truth-value reordering fixed four condition-
   building sites, but only three of them are reachable under observe or
   enforce.  The quick_enforce site is reached by no other test with a
   non-comparison predicate -- contracts-assert-quick-enforce.c uses
   `x > 0`, which is exactly the shape that never had the bug -- so that
   site had no regression coverage at all.  */

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
