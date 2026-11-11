// The implicit (*this) transformation is ill-formed in the initializer of a
// postcondition capture of a constructor (P3098 [expr.prim.id.general]), as
// it is in a constructor precondition; the capture's name is not reported
// again in the predicate.  The destructor half is
// cdtor-capture-implicit-this-dtor.C.
//
// Mirror: clang/test/Contracts/OpenBugs/ctor-capture-implicit-this.cpp in the
// llvm_llvm-project fork (CLANG-547 there).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

struct A1 {
  int v = 0;
  A1 () post [c = v] (c == 0) {} // { dg-error "'this' required" }
};
