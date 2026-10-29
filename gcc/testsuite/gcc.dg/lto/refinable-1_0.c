/* The refinable mark is streamed: under LTO, facts derived from a
   refinable C99 inline definition are still not used for calls of the
   symbol, which bind to the external definition in another unit (here
   outside the IR, and different).  */
/* { dg-lto-do run } */
/* { dg-lto-options { { -O2 -std=gnu99 -fno-inline -flto --param=ipa-refinable-inline=1 } } } */

inline int f (void) { return 1; }

int
main (void)
{
  /* The external definition returns 2.  */
  return f () == 2 ? 0 : 1;
}
