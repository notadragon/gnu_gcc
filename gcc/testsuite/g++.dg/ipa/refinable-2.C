// ipa-reference does not use what a refinable body reads or writes for
// calls of the symbol that are not inlined.
// { dg-do compile }
// { dg-options "-O2 -fno-ipa-modref -fno-ipa-icf -fdump-tree-optimized --param=ipa-refinable-inline=1" }

#define NI [[gnu::noinline]]

// ipa-reference tracks variables local to the unit.  Neither body touches
// the variable it is paired with; the store to another one keeps them from
// being pure.  M_* is marked, U_* is the unmarked control.
static int gr1, gr2, go1, go2;
NI inline void m_ref () { go1 = 1; }
template <int> NI void u_ref () { go2 = 1; }
int use_m_ref () { gr1 = 0; m_ref (); return gr1 ? 1001 : 0; }
int use_u_ref () { gr2 = 0; u_ref<0> (); return gr2 ? 2001 : 0; }

// { dg-final { scan-tree-dump "1001" "optimized" } }
// { dg-final { scan-tree-dump-not "2001" "optimized" } }
