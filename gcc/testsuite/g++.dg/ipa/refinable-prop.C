// A function that inlines a refinable body becomes refinable itself.
// { dg-do compile }
// { dg-options "-O2 -fno-ipa-icf -fdump-tree-optimized -fdump-ipa-inline --param=ipa-refinable-inline=1" }

#define NI [[gnu::noinline]]

// Marked by the parameter (declared inline), small: inlined everywhere.
inline int m_leaf (int x) { return x > 0 ? 1 : 2; }

// Not declared inline, so not marked by the parameter; U_MID becomes
// refinable by inlining M_LEAF.
template <int> NI int u_mid (int x) { return m_leaf (x); }
int use_mid (int x) { return u_mid<0> (x) > 5 ? 1001 : 0; }

// Control: the same COMDAT linkage without the inlined refinable body.
template <int> NI int u_ctl (int x) { return x > 0 ? 3 : 4; }
int use_ctl (int x) { return u_ctl<0> (x) > 5 ? 2001 : 0; }

// A local function that inlines M_LEAF is marked, but all its calls are
// visible, so its facts are still used.
NI static int s_mid (int x) { return m_leaf (x) + 4; }
int use_s_mid (int x) { return s_mid (x) > 9 ? 2002 : 0; }

// A strong external definition is the only definition of its symbol, so
// its facts are used even though it inlined M_LEAF.
NI int e_mid (int x) { return m_leaf (x) + 6; }
int use_e_mid (int x) { return e_mid (x) > 9 ? 2003 : 0; }

// { dg-final { scan-tree-dump "1001" "optimized" } }
// { dg-final { scan-tree-dump-not "2001" "optimized" } }
// { dg-final { scan-tree-dump-not "2002" "optimized" } }
// { dg-final { scan-tree-dump-not "2003" "optimized" } }
// { dg-final { scan-ipa-dump "u_mid\\(int\\) \[^\n\]*\n(  \[^\n\]*\n){0,5}  Availability: overwritable \\(refinable body\\)" "inline" } }
// { dg-final { scan-ipa-dump "e_mid\\(int\\)\\)\n(  \[^\n\]*\n){0,5}  Availability: available \\(refinable body\\)" "inline" } }
