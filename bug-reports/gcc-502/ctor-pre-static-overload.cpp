// An unqualified call in a constructor precondition (or destructor
// postcondition) is ill-formed only "if overload resolution selects a
// non-static member function" ([over.call.func]/3).  A call whose overload set
// mixes static and non-static members, and which selects the static one, is
// well-formed, but GCC rejects it before overload resolution.
//
// g++ -std=c++26 -fcontracts -fsyntax-only ctor-pre-static-overload.cpp
//   -> error: 'S::mixed' 'this' required when accessing a member within a
//      constructor precondition or destructor postcondition contract check
//      (lines 16 and 17).  Expected: no diagnostic.
struct S {
  bool mixed(int) const;
  static bool mixed(double);
  static bool sf(int);
  S(short) pre(sf(1));           // OK, accepted (control)
  S(char) pre(mixed(1.0));       // OK, selects the static overload
  ~S() post(mixed(1.0));         // OK, selects the static overload
};
