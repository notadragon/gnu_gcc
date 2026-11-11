// An unqualified call in a constructor precondition or destructor
// postcondition is ill-formed only "if overload resolution selects a
// non-static member function" ([over.call.func]/3): a call that selects the
// static member of a mixed set is fine, one that selects a non-static member
// is not.
//
// Mirror: clang/test/Contracts/ctor-pre-static-overload.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct S {
  bool mixed (int) const;
  static bool mixed (double);
  static bool sf (int);
  S (short) pre (sf (1));
  S (char) pre (mixed (1.0));
  ~S () post (mixed (1.0));
  S (int) pre (mixed (1));	// { dg-error "without object" }
};
