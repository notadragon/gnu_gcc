/* Facts derived from a refinable C99 inline definition or gnu_inline
   body are not used for calls of the symbol, which bind to the external
   definition in another unit.  */
/* { dg-do compile } */
/* { dg-options "-O2 -std=gnu99 -fno-inline -fexceptions -fdump-tree-optimized --param=ipa-refinable-inline=1" } */

/* Return range.  */
inline int c99_range (int x) { return x > 0 ? 1 : 2; }
int use_c99_range (int x) { return c99_range (x) > 5 ? 1001 : 0; }

extern inline __attribute__ ((gnu_inline)) int
gi_range (int x)
{
  return x > 0 ? 1 : 2;
}
int use_gi_range (int x) { return gi_range (x) > 5 ? 1002 : 0; }

/* Const.  */
inline int c99_const (int x) { return x * 3; }
int use_c99_const (int x) { return c99_const (x) + c99_const (x); }

/* Nothrow: the call keeps its landing pad for the cleanup.  */
void cleanup (int *);
inline int c99_nothrow (int x) { return x + 1; }
int
use_c99_nothrow (int x)
{
  int y __attribute__ ((cleanup (cleanup))) = x;
  return c99_nothrow (y);
}

/* An external definition of an inline function in this unit is the only
   definition of its symbol, so its facts are used although it is marked
   too.  */
inline int ext_range (int x) { return x > 0 ? 3 : 4; }
extern int ext_range (int);
int use_ext_range (int x) { return ext_range (x) > 5 ? 2001 : 0; }

/* { dg-final { scan-tree-dump "1001" "optimized" } } */
/* { dg-final { scan-tree-dump "1002" "optimized" } } */
/* { dg-final { scan-tree-dump-times "= c99_const \\\(" 2 "optimized" } } */
/* { dg-final { scan-tree-dump "resx|__builtin_eh_pointer|\\\[LP " "optimized" } } */
/* { dg-final { scan-tree-dump-not "2001" "optimized" } } */
