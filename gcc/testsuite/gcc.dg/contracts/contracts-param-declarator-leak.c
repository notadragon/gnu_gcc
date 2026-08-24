/* A contract written on a parameter's own function declarator must not be
   attached to the enclosing function.

   Regression test: pending_contracts is a file-static global filled while
   parsing a declarator, and c_parser_direct_declarator_inner parses
   contract specifiers on *any* function declarator -- including one
   nested inside a parameter list.  When the enclosing function had no
   contract of its own, nothing ever truncated the globals, so the
   parameter's contract was injected into the enclosing function body.
   That is not a lost contract but a wrong one: `_Pre (0)` fired against a
   function that never had a precondition.

   C++ accepts and ignores the same construct, so ignoring is the right
   behaviour; it is now warned about rather than dropped silently.  */

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

/* The enclosing function has no contract; the parameter's must not fire.  */
int
no_contract_of_its_own (void (*cb) (int) _Pre (0), int v) /* { dg-warning "contract on a parameter declarator is ignored" } */
{
  (void) cb;
  return v;
}

/* The enclosing function has its own contract; only that one may fire,
   and it must not be displaced by the parameter's.  */
int
has_own_contract (void (*cb) (int) _Pre (0), int v) _Pre (v > 0) /* { dg-warning "contract on a parameter declarator is ignored" } */
{
  (void) cb;
  return v;
}

/* Control: an ordinary function-pointer parameter with no contract.  */
int
plain (void (*cb) (int), int v) _Pre (v > 0)
{
  (void) cb;
  return v;
}

int
main (void)
{
  violations = 0;
  no_contract_of_its_own (0, 1);
  if (violations != 0)
    __builtin_abort ();

  violations = 0;
  has_own_contract (0, 1);      /* v > 0 holds */
  if (violations != 0)
    __builtin_abort ();

  violations = 0;
  has_own_contract (0, -1);     /* v > 0 violated, exactly once */
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  plain (0, -1);
  if (violations != 1)
    __builtin_abort ();

  return 0;
}
