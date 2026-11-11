// Without the mark, the facts derived from the same bodies are used: the
// control for refinable-1.C.
// { dg-do compile }
// { dg-options "-O2 -fno-ipa-icf -fno-ipa-sra -fdump-tree-optimized -fdump-ipa-inline" }

#include "refinable-1.C"

// { dg-final { scan-tree-dump-not "1001" "optimized" } }
// { dg-final { scan-tree-dump-not "2001" "optimized" } }
// { dg-final { scan-tree-dump-times "= m_const \\\(" 1 "optimized" } }
// { dg-final { scan-tree-dump-times "= u_const<0> \\\(" 1 "optimized" } }
// { dg-final { scan-tree-dump-not "1003" "optimized" } }
// { dg-final { scan-tree-dump-not "2003" "optimized" } }
// { dg-final { scan-tree-dump-not "1004" "optimized" } }
// { dg-final { scan-tree-dump-not "2004" "optimized" } }
// { dg-final { scan-tree-dump-not "1005" "optimized" } }
// { dg-final { scan-tree-dump-not "2005" "optimized" } }
// { dg-final { scan-tree-dump-not "1006" "optimized" } }
// { dg-final { scan-tree-dump-not "2006" "optimized" } }
// { dg-final { scan-ipa-dump-not "refinable body" "inline" } }
