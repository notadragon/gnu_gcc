/* Extra translation unit of refinable-body-ignore-replaceable-c99.C, compiled
   as C: the C99 inline definition of f, its assertion ignored, and a caller
   compiled with it in view.  An inline definition is never emitted, so the
   call goes to the external definition in -c99-e.c.  */

#include <contracts.h>

extern int cleanups;
static void cleanup_fn (int *p) { (void) p; ++cleanups; }

inline int f (int x)
{
  _ContractAssert (x > 0);	/* ignore */
  return x;
}

int use_cleanup (int y)
{
  int c __attribute__ ((cleanup (cleanup_fn))) = 0;
  return f (y);
}
