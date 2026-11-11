/* GCC-897: -O2 -fipa-pta -Wl,--defsym=keep=keep_escape

   The weak definition of KEEP below does not let *P escape, but the
   definition the program is linked with, KEEP_ESCAPE, does (the defsym
   stands in for a strong definition in another translation unit).  IPA
   points-to analysis models the call in USE by the weak body, so it finds
   that X does not escape, folds the read of X after OPAQUE to 1, and USE
   returns 1 instead of 2.  -O2 alone prints 2.  */
#include <stdio.h>

int g;
static int *saved;

__attribute__((weak, noinline)) void keep (int *p) { g = 1; }
__attribute__((noinline)) void keep_escape (int *p) { saved = p; }
__attribute__((noinline)) void opaque (void) { if (saved) *saved = 2; }

__attribute__((noinline)) int use (void)
{
  int x = 1;
  keep (&x);
  opaque ();
  return x;
}

int main (void)
{
  printf ("use () = %d (2 expected)\n", use ());
  return 0;
}
