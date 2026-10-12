// A destructor's postcondition capture initializer runs at entry, while the
// object is alive, so the implicit (*this) transformation is valid there
// (P3098 [expr.prim.id.general] forbids it only in a destructor's
// postcondition predicate).  The constructor half is
// cdtor-capture-implicit-this-ctor.C.
//
// Mirror: clang/test/Contracts/dtor-capture-implicit-this.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

struct A2 {
  int v = 0;
  ~A2 () post [c = v] (c == 0) {}
};
