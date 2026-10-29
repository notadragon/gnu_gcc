/* P3100 sanitizer routing is available in C, as in C++: under
   -fsanitize-semantic=address:noexcept_observe the stack overflow below
   calls the violation handler once and execution continues.

   Mirror: clang/test/Contracts/Runnable/p3100-c-routing.c in the
   llvm_llvm-project fork.  */
/* { dg-do run } */
/* { dg-additional-options "-std=c23 -fcontracts-p4299 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-semantic=address:noexcept_observe" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */
/* { dg-bogus ".-fcontracts-p3100. is valid for C\\+\\+" "" { target *-*-* } 0 } */
/* { dg-bogus ".-fcontracts-p4298. is valid for C\\+\\+" "" { target *-*-* } 0 } */
/* { dg-bogus "has no effect without" "" { target *-*-* } 0 } */

#include <contracts.h>

static int handler_called;

void handle_contract_violation (const contract_violation_t *cv)
{
  (void) cv;
  handler_called++;
}

volatile int sink;

int __attribute__((noinline))
oob (int i)
{
  int a[3] = { 1, 2, 3 };
  return a[i];			/* stack out-of-bounds read: i >= 5 */
}

int main (int argc, char **argv)
{
  (void) argv;
  sink = oob (argc + 4);
  /* noexcept_observe: the handler ran once and execution continued.  */
  if (handler_called != 1)
    __builtin_abort ();
  return 0;
}
