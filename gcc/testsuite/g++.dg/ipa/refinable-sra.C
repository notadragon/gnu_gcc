// IPA-SRA still clones a refinable body; the clone is local, so facts
// derived from it are used at the redirected calls.
// { dg-do compile }
// { dg-options "-O2 -fdump-ipa-sra -fdump-tree-optimized --param=ipa-refinable-inline=1" }

[[gnu::noinline]] inline int
m_sra (int a, int)
{
  return a > 0 ? 1 : 2;
}

int use (int x) { return m_sra (x, 5); }

// { dg-final { scan-ipa-dump "Created new node m_sra.isra" "sra" } }
// { dg-final { scan-tree-dump "m_sra.isra" "optimized" } }
