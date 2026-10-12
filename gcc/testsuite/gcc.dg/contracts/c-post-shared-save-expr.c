/* Each return site emits its own copy of the _Post predicate, so a SAVE_EXPR
   in it (here from GNU "?:") is evaluated on every return path: f (3) takes
   the second return and count () is still called there.

   Mirror: clang/test/Contracts/c-post-shared-save-expr.c in the
   llvm_llvm-project fork.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

static int calls = 0;
static int count (void) { return ++calls; }

int f (int x) _Post (r: __extension__ (count () ?: 1) > 0)
{
  if (x > 10)
    return 1;
  return x;
}

int main (void)
{
  return f (3) == 3 && calls == 1 ? 0 : 1;
}
