// In a constructor precondition or destructor postcondition a potentially
// evaluated qualified-id naming a non-static member (C::b) is transformed to
// a class member access and is ill-formed ([expr.prim.id.general]), like the
// unqualified b (GCC-574); forming a pointer to member is not such a use.
//
// Mirror: clang/test/Contracts/ctor-pre-qualified-member-rejected.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct C {
  bool b;
  C(char) pre(C::b);  // { dg-error "'this' required" }
  C(short) pre(&C::b != nullptr);     // OK, pointer to member
};

struct E {
  bool b;
  ~E() post(E::b);    // { dg-error "'this' required" }
};
