// Facts derived from a refinable body are not used for calls of the
// symbol that are not inlined.
// { dg-do compile }
// { dg-options "-O2 -fno-ipa-icf -fno-ipa-sra -fdump-tree-optimized -fdump-ipa-inline --param=ipa-refinable-inline=1" }

// IPA-ICF and IPA-SRA are disabled: they would merge the bodies or make
// local clones of them, whose facts may be used.
//
// The parameter marks the bodies of the functions declared inline (M_*)
// as refinable.  The templates (U_*) are COMDAT as well, but they are not
// declared inline, so they stay unmarked: they are the controls, and the
// facts derived from their bodies are used.

#define NI [[gnu::noinline]]

// Return range.
NI inline int m_range (int x) { return x > 0 ? 1 : 2; }
template <int> NI int u_range (int x) { return x > 0 ? 3 : 4; }
int use_m_range (int x) { return m_range (x) > 5 ? 1001 : 0; }
int use_u_range (int x) { return u_range<0> (x) > 5 ? 2001 : 0; }

// Const.
NI inline int m_const (int x) { return x * 3; }
template <int> NI int u_const (int x) { return x * 5; }
int use_m_const (int x) { return m_const (x) + m_const (x); }
int use_u_const (int x) { return u_const<0> (x) + u_const<0> (x); }

// Noreturn.
NI inline void m_noret () { __builtin_abort (); }
template <int> NI void u_noret () { __builtin_trap (); }
int use_m_noret () { m_noret (); return 1003; }
int use_u_noret () { u_noret<0> (); return 2003; }

// Malloc.
NI inline int *m_malloc () { return (int *) __builtin_malloc (4); }
template <int> NI int *u_malloc () { return (int *) __builtin_malloc (8); }
int use_m_malloc (int *q) { int *p = m_malloc (); *q = 7; *p = 0;
			    return *q == 7 ? 0 : 1004; }
int use_u_malloc (int *q) { int *p = u_malloc<0> (); *q = 7; *p = 0;
			    return *q == 7 ? 0 : 2004; }

// Nonnull return.
int g1, g2;
NI inline int *m_nonnull () { return &g1; }
template <int> NI int *u_nonnull () { return &g2; }
int use_m_nonnull () { return m_nonnull () ? 0 : 1005; }
int use_u_nonnull () { return u_nonnull<0> () ? 0 : 2005; }

// Mod-ref: the bodies do not touch *P and do not let it escape.
int gm1, gm2;
NI inline void m_modref (int *) { gm1 = 1; }
template <int> NI void u_modref (int *) { gm2 = 1; }
int use_m_modref () { int x = 0; m_modref (&x); return x ? 1006 : 0; }
int use_u_modref () { int x = 0; u_modref<0> (&x); return x ? 2006 : 0; }

// { dg-final { scan-tree-dump "1001" "optimized" } }
// { dg-final { scan-tree-dump-not "2001" "optimized" } }
// { dg-final { scan-tree-dump-times "= m_const \\\(" 2 "optimized" } }
// { dg-final { scan-tree-dump-times "= u_const<0> \\\(" 1 "optimized" } }
// { dg-final { scan-tree-dump "1003" "optimized" } }
// { dg-final { scan-tree-dump-not "2003" "optimized" } }
// { dg-final { scan-tree-dump "1004" "optimized" } }
// { dg-final { scan-tree-dump-not "2004" "optimized" } }
// { dg-final { scan-tree-dump "1005" "optimized" } }
// { dg-final { scan-tree-dump-not "2005" "optimized" } }
// { dg-final { scan-tree-dump "1006" "optimized" } }
// { dg-final { scan-tree-dump-not "2006" "optimized" } }
// The IPA dumps show the availability.
// { dg-final { scan-ipa-dump "Availability: overwritable \\\(refinable body\\\)" "inline" } }
