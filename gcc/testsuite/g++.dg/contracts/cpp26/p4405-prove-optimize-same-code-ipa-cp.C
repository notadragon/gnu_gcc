// D4405's proof optimization level (DECISIONS.md E162-E168, E206): at a
// fixed proof level the real code is the same, whatever the level it is
// generated at, as without the shadows and with every evaluation ignore.
// The proof shadows are optimized at level 2 and call level copies of
// their callees where those are at another level (-O0, -O1, -Og, -O3,
// -Os, -Oz); the copies are analysis-only code, like the shadows.  The shape
// is p4405-shadow-same-code-ipa-cp.C's:
// IPA-CP's clones.  The dg-final code compiles it at each level with ignore, with
// the shadows and without them, and compares the object code; at -O0
// also at proof level 0 and with -fcontracts-prove-assume, which assumes
// nothing there (DECISIONS.md E167); at -O0 and -O2 also with every
// strategy given explicitly at proof level 0, where the shadows run at
// level 0 (E208 (a)); at -O1 and -O2 also with -g, the debug sections
// included (GCC-997).
//
// Mirror: clang/test/Contracts/p4405-prove-optimize-same-code.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2 -fcontracts-prove-optimize=2" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

#include "p4405-shadow-same-code-ipa-cp.C"

// { dg-final { contracts-same-object-code "-O0 -fcontract-evaluation-semantic=ignore" "-O0" "-O0 -fno-contracts-prove-shadow" "-O0 -fcontracts-prove-optimize=0" "-O0 -fcontracts-prove-assume" "-O0 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -fcontracts-prove-assume" } }
// { dg-final { contracts-same-object-code "-O1 -fcontract-evaluation-semantic=ignore" "-O1" "-O1 -fno-contracts-prove-shadow" } }
// { dg-final { contracts-same-object-code "-Og -fcontract-evaluation-semantic=ignore" "-Og" "-Og -fno-contracts-prove-shadow" } }
// { dg-final { contracts-same-object-code "-O2 -fcontract-evaluation-semantic=ignore" "-O2" "-O2 -fno-contracts-prove-shadow" "-O2 -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises" } }
// { dg-final { contracts-same-object-code "-O3 -fcontract-evaluation-semantic=ignore" "-O3" "-O3 -fno-contracts-prove-shadow" } }
// { dg-final { contracts-same-object-code "-Os -fcontract-evaluation-semantic=ignore" "-Os" "-Os -fno-contracts-prove-shadow" } }
// { dg-final { contracts-same-object-code "-Oz -fcontract-evaluation-semantic=ignore" "-Oz" "-Oz -fno-contracts-prove-shadow" } }
// { dg-final { contracts-same-object-code "-O1 -g -gno-record-gcc-switches -fcontract-evaluation-semantic=ignore" "-O1 -g -gno-record-gcc-switches" "-O1 -g -gno-record-gcc-switches -fno-contracts-prove-shadow" } }
// { dg-final { contracts-same-object-code "-O2 -g -gno-record-gcc-switches -fcontract-evaluation-semantic=ignore" "-O2 -g -gno-record-gcc-switches" "-O2 -g -gno-record-gcc-switches -fno-contracts-prove-shadow" } }
