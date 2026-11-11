/* GCC-996, as a run: build with -O1 and run.  After f, which its optimize
   attribute compiles at -O2, every function compiled at -O1 keeps -O2's
   -falign-functions.  Each t<N> is one instruction at -O1, so without that
   alignment they are packed and few start on a 16-byte boundary (stock
   13.4.0 and 14.4.0 print aligned=1/8); with it every one does
   (aligned=8/8).  */
#include <stdio.h>
#include <stdint.h>

int sink (int);
__attribute__ ((optimize ("O2"))) int
f (int n)
{
  int s = 0;
  for (int i = 0; i < n; ++i)
    s += sink (i);
  return s;
}

int sink (int i) { return i; }
void t0 (void) {}
void t1 (void) {}
void t2 (void) {}
void t3 (void) {}
void t4 (void) {}
void t5 (void) {}
void t6 (void) {}
void t7 (void) {}

int
main (void)
{
  void (*t[]) (void) = { t0, t1, t2, t3, t4, t5, t6, t7 };
  int aligned = 0;
  for (int i = 0; i < 8; ++i)
    aligned += ((uintptr_t) t[i] % 16) == 0;
  printf ("f=%d aligned=%d/8\n", f (3), aligned);
  return 0;
}
