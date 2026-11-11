// A block-scope function declaration that is the first declaration carries a
// precondition, and the namespace-scope definition repeats it, which is
// valid (GCC-551).  The dropped-contract shape is
// block-scope-first-decl-contract.C.
//
// Mirror: clang/test/Contracts/block-scope-decl-then-def-contract.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void o (int t)
{
  void loc (int x) pre (x > 0);
  loc (t);
}

void loc (int x) pre (x > 0) {}
