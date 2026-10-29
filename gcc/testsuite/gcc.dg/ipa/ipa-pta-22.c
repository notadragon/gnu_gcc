/* With semantic interposition, IPA points-to analysis must not use the
   body of an exported function when it analyzes a call of it.  */
/* { dg-do compile { target fpic } } */
/* { dg-require-visibility "" } */
/* { dg-options "-O2 -fPIC -fsemantic-interposition -fipa-pta -fno-ipa-sra -fno-ipa-modref -fdump-tree-optimized" } */

void opaque (void);

/* Neither body lets *P escape, but another definition of E_KEEP may be
   bound in its place at run time.  H_KEEP is hidden, so it may not.  */
int g1, g2;
__attribute__((noinline)) void e_keep (int *p) { g1 = 1; }
__attribute__((noinline, visibility ("hidden"))) void h_keep (int *p) { g2 = 1; }

int use_e (void) { int x = 1; e_keep (&x); opaque (); return x == 1 ? 0 : 1001; }
int use_h (void) { int x = 1; h_keep (&x); opaque (); return x == 1 ? 0 : 2001; }

/* { dg-final { scan-tree-dump "1001" "optimized" } } */
/* { dg-final { scan-tree-dump-not "2001" "optimized" } } */
