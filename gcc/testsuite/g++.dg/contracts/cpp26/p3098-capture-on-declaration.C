// A postcondition capture (P3098) on a declaration that is not the
// definition is initialized from the definition's parameter, not the
// declaration's, which has no value in the definition (GCC-658).  The
// parameters are named differently here so that the dump shows which one
// the initializer reads; p3098-capture-on-declaration-run.C runs it.
//
// Mirror: clang/test/Contracts/Runnable/p3098-capture-on-declaration.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe -fdump-tree-original" }

int g (int a) post [old = a] (old > 0);
int g (int y) { return y; }

// { dg-final { scan-tree-dump "old = \\(const int\\) y" "original" } }
