/* P4405 in C: the prove semantic without -fcontracts-p4405 is not allowed;
   the compiler says so, and ignore is used instead, so the predicate is not
   evaluated and nothing is diagnosed.

   Mirror: clang/test/Contracts/C/c-p4405-without-flag.c in the
   llvm_llvm-project fork.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=prove" } */
/* { dg-warning "requires .-fcontracts-p4405.; .ignore. is used instead" "" { target *-*-* } 0 } */

#include <contracts.h>

static int evaluations;

static int
positive (int x)
{
  ++evaluations;
  return x > 0;
}

int guarded (int x) _Pre (positive (x)) { return x; }

int
main (void)
{
  guarded (-1);
  return evaluations;
}
