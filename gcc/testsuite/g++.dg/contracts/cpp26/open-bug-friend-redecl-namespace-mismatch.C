// Watch test for an OPEN bug (GCC-27, deferred): a friend declaration that
// redeclares an ordinary namespace-scope function with a different contract
// is not diagnosed unless a later declaration happens to flush the
// redeclaration-match queue; nothing after this class does.  Two friend
// declarations of one function are diagnosed:
// contract-friend-deferred-mismatch.C.
//
// Mirror: clang/test/Contracts/qualified-friend-member-contract-mismatch.cpp
// in the llvm_llvm-project fork (its struct A), which Clang passes
// (CLANG-639).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

int h (int x) pre (x > 0);

struct D
{
  friend int h (int x) pre (x < 0); // { dg-error "mismatched contract condition" "GCC-27" { xfail *-*-* } }
};

int h (int x) { return x; }
