// An external inline body (here an explicit instantiation declaration of
// an inline function template) is never the definition a call binds to,
// so facts derived from a refinable one are not used.
// { dg-do compile }
// { dg-options "-O2 -fno-inline -fdump-tree-optimized --param=ipa-refinable-inline=1" }

template <typename T> inline int m_ext (T x) { return x > 0 ? 1 : 2; }
extern template int m_ext<int> (int);
int use_m_ext (int x) { return m_ext (x) > 5 ? 1001 : 0; }

// { dg-final { scan-tree-dump "1001" "optimized" } }
