// A redeclaration repeating a capturing postcondition token for token has
// the same contract under P3098 (the captures and predicate satisfy the
// one-definition rule), so it is accepted.
//
// Mirror: clang/test/Contracts/redecl-capturing-postcondition.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

int f (int x) post [x] (r : r == x + 1);
int f (int x) post [x] (r : r == x + 1) { return x + 1; }
