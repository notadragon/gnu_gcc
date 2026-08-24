/* A contract on a non-defining declaration (a prototype, as in a header)
   must not leak into a subsequent contract-free definition (D4299).
   Previously the prototype's precondition was injected into the body of
   the next definition, breaking otherwise-valid code.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  (void) cv;
  handler_called++;
}

/* Prototype carrying a precondition.  */
int foo(int x) _Pre(x > 0);

/* A contract-free definition that must NOT inherit foo's precondition.  */
int bar(int x)
{
  return x;
}

/* The real definition of foo, without its own contract specifier.  */
int foo(int x)
{
  return x;
}

int main(void)
{
  /* bar has no contract of its own: even a "bad" argument must not
     trigger a violation.  */
  bar(-1);
  if (handler_called != 0)
    __builtin_abort();

  foo(-1);
  if (handler_called != 0)
    __builtin_abort();

  return 0;
}
