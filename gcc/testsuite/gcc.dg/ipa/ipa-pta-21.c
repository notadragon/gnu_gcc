/* IPA points-to analysis must not use the body of a callee whose
   definition may be interposed, or whose body is not to be used for
   interprocedural analysis, when it analyzes a call of it.  */
/* { dg-do compile } */
/* { dg-require-weak "" } */
/* { dg-options "-O2 -fipa-pta -fno-ipa-sra -fno-ipa-modref -fno-semantic-interposition -fdump-tree-optimized" } */
/* IPA-SRA and mod-ref are disabled so that only IPA-PTA can find that *P
   does not escape.  */

void opaque (void);

/* No body below lets *P escape.  Another definition of W_KEEP may be
   linked in its place, and the body of N_KEEP is noipa; E_KEEP and S_KEEP
   are the controls.  The stores keep the bodies from being const.  */
int g1, g2, g3, g4;
__attribute__((weak, noinline)) void w_keep (int *p) { g1 = 1; }
__attribute__((noipa)) void n_keep (int *p) { g2 = 1; }
__attribute__((noinline)) void e_keep (int *p) { g3 = 1; }
static __attribute__((noinline)) void s_keep (int *p) { g4 = 1; }

int use_w (void) { int x = 1; w_keep (&x); opaque (); return x == 1 ? 0 : 1001; }
int use_n (void) { int x = 1; n_keep (&x); opaque (); return x == 1 ? 0 : 1002; }
int use_e (void) { int x = 1; e_keep (&x); opaque (); return x == 1 ? 0 : 2001; }
int use_s (void) { int x = 1; s_keep (&x); opaque (); return x == 1 ? 0 : 2002; }

/* X escapes in the calls of W_KEEP and N_KEEP, so OPAQUE may change it.  */
/* { dg-final { scan-tree-dump "1001" "optimized" } } */
/* { dg-final { scan-tree-dump "1002" "optimized" } } */
/* It does not escape in the calls of E_KEEP and S_KEEP.  */
/* { dg-final { scan-tree-dump-not "2001" "optimized" } } */
/* { dg-final { scan-tree-dump-not "2002" "optimized" } } */
