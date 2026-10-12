/* The parameter-list contract shield must not lose the enclosing function's
   own contract.

   c_parser_direct_declarator_inner parses contract specifiers on any
   function declarator, including one nested inside a parameter list, and
   pending_contracts is a file-static global -- so the parameter-list parse
   saves those globals, parses into a fresh pair, and restores.  This checks
   the restore: a function whose parameter is a function pointer still gets
   its own _Pre, and gets it exactly once.

   A contract on the parameter's own declarator is ill-formed, in C as in
   C++; those shapes are in contracts-non-function-declarator.c.  */

/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */

#include <contracts.h>

static int violations = 0;

void
handle_contract_violation (const contract_violation_t *v)
{
  (void) v;
  ++violations;
}

/* Function-pointer parameter, and a contract of the function's own.  */
int
with_fnptr_parm (void (*cb) (int), int v) _Pre (v > 0)
{
  (void) cb;
  return v;
}

/* Two of them, so the shield is entered and left more than once in one
   declarator.  */
int
two_fnptr_parms (void (*a) (int), void (*b) (long), int v) _Pre (v > 0)
{
  (void) a;
  (void) b;
  return v;
}

/* No parameter list to shield: the control for the control.  */
int
no_parms (int v) _Pre (v > 0)
{
  return v;
}

int
main (void)
{
  violations = 0;
  with_fnptr_parm (0, 1);
  if (violations != 0)
    __builtin_abort ();

  violations = 0;
  with_fnptr_parm (0, -1);
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  two_fnptr_parms (0, 0, -1);
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  no_parms (-1);
  if (violations != 1)
    __builtin_abort ();

  return 0;
}
