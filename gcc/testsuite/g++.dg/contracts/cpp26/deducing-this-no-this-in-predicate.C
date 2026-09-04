// `this` is unavailable inside an explicit object member function
// ([expr.prim.this]/1), and a contract predicate on such a function is inside
// it.  Naming `this` -- explicitly, or implicitly by naming a non-static data
// member unqualified, which means (*this).m -- must be rejected.
//
// GCC rejects both.  Clang accepts both and then asserts in CodeGen
// (LoadCXXThis: "no 'this' value for this function"), while correctly
// rejecting the identical use in the function BODY -- so the hole there is
// contracts-specific.  The Clang mirror of this file is XFAILed.
//
// The unqualified-member diagnostics below are DEFECTIVE, and the xfail'd
// expectations record what they should say.  GCC reports every one of them as
// "'this' required when accessing a member within a constructor precondition
// or destructor postcondition contract check", but none of these functions is
// a constructor or a destructor -- each is an explicit object member function,
// for which GCC already has the right message and uses it for the explicit
// `this` case.  It is wrong in a precondition, in a postcondition, and in an
// assertion-statement in the body alike.  When that is fixed, drop the xfails.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// Diagnostics inside an explicit object member function carry a context line
// that DejaGnu counts as output; g++.dg/cpp23/explicit-obj-diagnostics4.C
// declares it with a line-0 dg-message, which matches one occurrence.  There
// are several here.
// { dg-prune-output "In explicit object member function" }

struct S
{
  int x = 0;
};

// Explicit `this` in a precondition.
struct ExplicitThis : S
{
  void f (this ExplicitThis &self) pre (this->x == 0); // { dg-error "'this' is unavailable for explicit object member functions" }
};

// An unqualified non-static data member in a precondition.
struct ImplicitThisPre : S
{
  void f (this ImplicitThisPre &self) pre (x == 0); // { dg-error "'this' required when accessing a member within a constructor precondition or destructor postcondition contract check" }
  // { dg-error "'this' is unavailable for explicit object member functions" "unqualified member should use the explicit-object-member-function message" { xfail *-*-* } .-1 }
};

// The same in a postcondition.
struct ImplicitThisPost : S
{
  int f (this ImplicitThisPost &self) post (r : x == r); // { dg-error "'this' required when accessing a member within a constructor precondition or destructor postcondition contract check" }
  // { dg-error "'this' is unavailable for explicit object member functions" "unqualified member should use the explicit-object-member-function message" { xfail *-*-* } .-1 }
};

// The same in an assertion-statement in the body.
struct ImplicitThisAssert : S
{
  void
  f (this ImplicitThisAssert &self)
  {
    contract_assert (x == 0); // { dg-error "'this' required when accessing a member within a constructor precondition or destructor postcondition contract check" }
    // { dg-error "'this' is unavailable for explicit object member functions" "unqualified member should use the explicit-object-member-function message" { xfail *-*-* } .-1 }
  }
};

// CONTROL: the explicit object parameter itself is of course usable.
struct ViaSelf : S
{
  void f (this ViaSelf &self) pre (self.x == 0);
};

// CONTROL: an ordinary (implicit object) member function may name `this` and
// its members in a predicate.
struct ImplicitObject : S
{
  void g () const pre (this->x == 0) pre (x == 0);
};
