// In a constructor precondition or destructor postcondition an unevaluated
// use of a non-static member is valid, as in the example in
// [expr.prim.id.general] (`C(int) pre(sizeof(b) > 0);  // OK`): the
// restriction applies only when the id-expression is potentially evaluated
// (GCC-574).  The qualified form is ctor-pre-qualified-member.C.
//
// Mirror: clang/test/Contracts/ctor-pre-unevaluated-member.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct C {
  bool b;
  int f() const;
  C(int) pre(sizeof(b) > 0);          // OK, the standard's example
  C(float) pre(sizeof(f()) > 0);      // OK, unevaluated
  C(double) pre(noexcept(f()));       // OK, unevaluated
  C(unsigned) pre(requires { b; });   // OK, unevaluated
  C(long) pre(&this->b);              // OK
};

struct D {
  bool b;
  ~D() post(sizeof(b) > 0);           // OK, unevaluated
};
