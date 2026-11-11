// D4405's proof optimization level (DECISIONS.md E162-E168, E205, E206):
// p4405-shadow-callee-premises.C in a -Og build, at the default proof
// level, 2.  The callees are optimized at -Og and the shadows at level 2,
// so the premise shadows call premise copies, and the ordinary shadows
// level copies, at level 2: each evaluation is proven as at -O2, the
// callees' premises reaching the premise shadows (h1, h3, h4, h5) without
// being declared inline (p4405-shadow-callee-premises-O1.C).
//
// Mirror: none.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -Og" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe -fdump-tree-contract_shadow" }

#include "p4405-shadow-callee-premises.C"

// { dg-final { scan-tree-dump {contract obligation [0-9]+ \(line 30\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {callee premises: obligation [0-9]+ \(line 30\): proven, conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation [0-9]+ \(line 36\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {callee premises: obligation [0-9]+ \(line 47\): proven, conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {callee premises: obligation [0-9]+ \(line 54\): proven, conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {callee premises: obligation [0-9]+ \(line 61\): proven, conditional} "contract_shadow" } }
