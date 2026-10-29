// The implicit (*this). transformation of a member named inside a lambda in
// a constructor precondition still occurs in the predicate
// ([expr.prim.id.general]), so it is ill-formed; naming it through an
// explicit this, or a local class's own member, is fine.
//
// Mirror: clang/test/Contracts/ctor-pre-member-in-lambda.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct S {
  int m;
  S (unsigned) pre ([&] { return m > 0; } ());		// { dg-error "'this' required" }
  S (long) pre ([this] { return this->m > 0; } ());
  S (char) pre ([] { struct L { int q; int get () { return q; } }; return true; } ());
  S (int) pre (m > 0);					// { dg-error "'this' required" }
  bool f () const;
  S (short) pre ([&] { return f (); } ());		// { dg-error "'this' required" }
  ~S () post ([&] { return m > 0; } ());		// { dg-error "'this' required" }
};
