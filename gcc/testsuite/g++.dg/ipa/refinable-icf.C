// Identical function bodies are not merged across the refinable mark.
// { dg-do compile }
// { dg-options "-O2 -fdump-ipa-icf-details --param=ipa-refinable-inline=1" }

#define NI [[gnu::noinline]]

// M_A is marked (declared inline); S_A and S_B are not.  All three bodies
// are identical.  Calls of S_A must not be redirected to M_A, whose
// definition in another unit may differ.
NI inline int m_a (int x) { return x * 7 + 3; }
NI static int s_a (int x) { return x * 7 + 3; }
NI static int s_b (int x) { return x * 7 + 3; }
int use (int x) { return m_a (x) + s_a (x) + s_b (x); }

// Only the two unmarked bodies are merged.
// { dg-final { scan-ipa-dump "Semantic equality hit:\[^\\n\]*s_\[ab\]\[^\\n\]*->\[^\\n\]*s_\[ab\]" "icf" } }
// { dg-final { scan-ipa-dump-not "Semantic equality hit:\[^\\n\]*m_a" "icf" } }
