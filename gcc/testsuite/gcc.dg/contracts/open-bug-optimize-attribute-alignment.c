/* Watch test for an open bug, GCC-996 in this repository's bug-reports/:
   in a -O1 compilation, after f, which its optimize attribute compiles at
   -O2 with -O2's -falign-functions and -falign-loops, g -- a plain -O1
   function -- keeps that alignment, since -O1 sets none and nothing resets
   it (parse_N_M in toplev.cc).  Stock GCC 15 and later; 13.4 and 14.4 give
   g no alignment, so only f's five .p2align directives are expected.  Not a
   contracts bug: the proof shadows of D4405's proof optimization level
   would have changed the real code's alignment through it, and avoid it by
   keeping the command line's alignment in their optimization options.  */
/* { dg-do compile { target { i?86-*-* x86_64-*-* } } } */
/* { dg-options "-O1" } */
/* { dg-skip-if "pinned to -O1" { *-*-* } { "-O0" "-O2" "-O3" "-Os" "-Og" "-flto" } { "" } } */

int sink (int);

__attribute__ ((optimize ("O2"))) int
f (int n)
{
  int s = 0;
  for (int i = 0; i < n; ++i)
    s += sink (i);
  return s;
}

int
g (int n)
{
  int s = 0;
  for (int i = 0; i < n; ++i)
    s += sink (i);
  return s;
}

/* { dg-final { scan-assembler-times {\.p2align} 5 { xfail *-*-* } } } */
