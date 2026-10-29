// A refinable body is still inlined, including always_inline, and the
// facts in the inlined copy are used within the caller.
// { dg-do compile }
// { dg-options "-O2 -Winline -fdump-tree-optimized --param=ipa-refinable-inline=1" }

[[gnu::always_inline]] inline int
m_always (int x)
{
  if (x < 0)
    __builtin_unreachable ();
  return x;
}

inline int
m_plain (int x)
{
  return x & 3;
}

int use_always (int x) { return m_always (x) >= 0 ? 0 : 1001; }
int use_plain (int x) { return m_plain (x) < 4 ? 0 : 1002; }

// { dg-final { scan-tree-dump-not "1001" "optimized" } }
// { dg-final { scan-tree-dump-not "1002" "optimized" } }
// { dg-final { scan-tree-dump-not "m_always" "optimized" } }
// { dg-final { scan-tree-dump-not "m_plain" "optimized" } }
