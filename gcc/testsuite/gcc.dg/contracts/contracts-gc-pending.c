/* The contracts the C parser holds between a declarator and the function
   body it guards -- pending_contracts with its result names, and the
   pending_contract_tokens they are replayed from -- live in file-static
   vectors that are not GC roots.  With every collection opportunity
   collecting, on an old-style definition, around a nested function, and
   with result names, they must still be intact when replayed (GCC-167,
   confirmed not to be a defect on a checking build, which poisons freed
   memory).  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe -Wno-old-style-definition --param ggc-min-expand=0 --param ggc-min-heapsize=0" } */

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  (void) cv;
  handler_called++;
}

int kandr(a, b) _Pre(a > 0) _Post(r: r == a + b)
     int a;
     int b;
{
  return a + b;
}

int outer(int a) _Pre(a != 0) _Post(r: r > 0)
{
  int inner(int b) _Pre(b != 0) _Post(s: s > 0)
  {
    return b;
  }
  return inner (a);
}

int named(int x) _Post(result: result >= x) _Post(other: other <= x + 1)
{
  return x + 1;
}

int main(void)
{
  if (kandr (1, 2) != 3 || outer (5) != 5 || named (1) != 2)
    __builtin_abort ();
  if (handler_called != 0)
    __builtin_abort ();
  kandr (-1, 2);
  outer (-5);
  if (handler_called != 3)
    __builtin_abort ();
  return 0;
}
