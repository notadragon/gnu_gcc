// IPA points-to analysis should not use the effects of a refinable body
// for calls of the symbol that are not inlined.
// { dg-do compile }
// { dg-options "-O2 -fipa-pta -fno-ipa-sra -fno-ipa-modref -fno-ipa-icf -fdump-tree-optimized --param=ipa-refinable-inline=1" }
// IPA-SRA and mod-ref are disabled so that only IPA-PTA can find that *P
// does not escape.

#define NI [[gnu::noinline]]

void opaque ();

// No body lets *P escape; another definition of M_KEEP or W_KEEP may.  The
// stores keep the bodies from being const.  M_KEEP is marked (declared
// inline), W_KEEP is weak and U_KEEP is the unmarked control.
int gk1, gk2, gk3;
NI inline void m_keep (int *) { gk1 = 1; }
template <int> NI void u_keep (int *) { gk2 = 1; }
[[gnu::weak]] NI void w_keep (int *) { gk3 = 1; }
int use_m_keep () { int x = 1; m_keep (&x); opaque (); return x == 1 ? 0 : 1001; }
int use_u_keep () { int x = 1; u_keep<0> (&x); opaque (); return x == 1 ? 0 : 2001; }
int use_w_keep () { int x = 1; w_keep (&x); opaque (); return x == 1 ? 0 : 1002; }

// IPA-PTA models a call of an interposable callee as a call of an
// unknown function, so *P escapes in the calls of M_KEEP and W_KEEP.
// { dg-final { scan-tree-dump "1001" "optimized" } }
// { dg-final { scan-tree-dump "1002" "optimized" } }
// { dg-final { scan-tree-dump-not "2001" "optimized" } }
