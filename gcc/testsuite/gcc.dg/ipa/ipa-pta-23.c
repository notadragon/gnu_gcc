/* IPA points-to analysis must not use the body of a weak definition when
   it analyzes a call of it: the definition linked in its place, here
   KEEP_ESCAPE, may let the argument escape.  */
/* { dg-do run { target *-*-linux* *-*-gnu* } } */
/* { dg-require-weak "" } */
/* { dg-options "-O2 -fipa-pta -Wl,--defsym=keep=keep_escape" } */

static int *saved;
int g;

__attribute__((weak, noinline)) void keep (int *p) { g = 1; }
__attribute__((noinline)) void keep_escape (int *p) { saved = p; }
__attribute__((noinline)) void opaque (void) { if (saved) *saved = 2; }

__attribute__((noinline)) int
use (void)
{
  int x = 1;
  keep (&x);
  opaque ();
  return x;
}

int
main (void)
{
  if (use () != 2)
    __builtin_abort ();
  return 0;
}
