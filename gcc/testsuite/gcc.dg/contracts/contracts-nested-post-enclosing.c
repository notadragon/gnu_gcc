/* Test that defining a GNU nested function does not discard the enclosing
   function's postconditions.

   Regression test: the nested-function finish-function branches cleared
   active_postconditions wholesale, but a nested function finishes while
   its enclosing function is still being parsed and still has its own
   entry live.  Every return site after the nested definition -- and the
   void implicit-return path -- then saw no active postcondition and got
   no check, silently and with no diagnostic.

   The nested functions below deliberately have no contracts of their own:
   merely defining one was enough to trigger this.

   Note the existing contracts-nested-post.c cannot catch it -- it only
   calls with satisfied postconditions, so it passes either way.  */

/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int violations = 0;

void
handle_contract_violation (const contract_violation_t *v)
{
  (void) v;
  ++violations;
}

/* A return that comes textually before the nested definition, and one
   that comes after: only the latter regressed.  */
int
split (int which) _Post (r : r > 0)
{
  if (which == 1)
    return -1;

  int inner (int b) { return b; }

  if (which == 2)
    return -1;

  return inner (-1);
}

/* The implicit return of a void function routes through a separate path
   in c-decl.cc and regressed too.  */
static int side = 0;

void
vpost (void) _Post (side > 0)
{
  int helper (int b) { return b; }
  side = helper (-1);
}

/* Control: same postcondition, no nested function anywhere.  */
int
plain (void) _Post (r : r > 0)
{
  return -1;
}

int
main (void)
{
  violations = 0;
  plain ();
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  split (1);
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  split (2);
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  split (3);
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  vpost ();
  if (violations != 1)
    __builtin_abort ();

  return 0;
}
