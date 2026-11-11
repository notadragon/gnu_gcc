// IPA-CP still specializes a refinable body: the clone is a local copy of
// it.
// { dg-do compile }
// { dg-options "-O3 -fno-inline -fdump-ipa-cp --param=ipa-refinable-inline=1 --param=ipa-cp-eval-threshold=1" }

int arr[64];

inline int
m_cp (int x, int n)
{
  int s = 0;
  for (int i = 0; i < n; i++)
    s += arr[i] * x;
  return s;
}

int use1 (int x) { return m_cp (x, 16); }
int use2 (int x) { return m_cp (x + 1, 16); }

// { dg-final { scan-ipa-dump "Creating a specialized node of \[^\\n\]*m_cp" "cp" } }
