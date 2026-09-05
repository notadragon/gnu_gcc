// gcc-28-xobj-member-in-predicate-ctor-message.cpp                  -*-C++-*-
//
// GCC-28: naming a non-static data member unqualified in a contract predicate
// of an EXPLICIT OBJECT member function is rejected with a message about
// constructors and destructors, for a function that is neither.
//
//   g++ -std=c++26 -fcontracts -fsyntax-only \
//       gcc-28-xobj-member-in-predicate-ctor-message.cpp
//
// Stock g++ 16.2 and trunk:
//
//   error: 'S::x' 'this' required when accessing a member within a
//   constructor precondition or destructor postcondition contract check
//
// Expected: the same diagnostic the function BODY gives for the same
// expression -- "invalid use of non-static data member 'S::x'", or for a
// member function call "cannot call member function ... without object".
//
// PLAIN -fcontracts.  No -fcontracts-p3850 and no other extension of ours is
// involved, which is what makes this upstream's.

struct S
{
  int x = 0;
  bool ok () const;
};

// (1) Unqualified data member in a PRECONDITION.
struct ImplicitThisPre : S
{
  void f (this ImplicitThisPre &self) pre (x == 0);
};

// (2) The same in a POSTCONDITION.
struct ImplicitThisPost : S
{
  int f (this ImplicitThisPost &self) post (r : x == r);
};

// (3) The same in an assertion-statement in the body.
struct ImplicitThisAssert : S
{
  void
  f (this ImplicitThisAssert &self)
  {
    contract_assert (x == 0);
  }
};

// (4) An unqualified MEMBER FUNCTION call in a predicate.  The second of the
// two guards with the same defect; this one had no test coverage at all.
struct ImplicitThisCall : S
{
  void f (this ImplicitThisCall &self) pre (ok ());
};

// CONTROL: the EXPLICIT `this` spelling was always right --
// "'this' is unavailable for explicit object member functions".  GCC had the
// correct words one line away, which is what made the wrong message
// conspicuous.
struct ExplicitThis : S
{
  void f (this ExplicitThis &self) pre (this->x == 0);
};

// CONTROL: naming the member through the explicit object parameter is of
// course fine, and must keep compiling.
struct ViaSelf : S
{
  void f (this ViaSelf &self) pre (self.x == 0) pre (self.ok ());
};

// CONTROL: an ordinary implicit-object member function may name `this` and
// its members unqualified.  This is what says the defect is specific to the
// explicit-object case rather than a general contracts restriction.
struct ImplicitObject : S
{
  void g () const pre (this->x == 0) pre (x == 0);
};

// CONTROL: a genuine constructor precondition is where that message belongs,
// and it must keep being produced there.
struct RealCtor : S
{
  RealCtor () pre (x == 0);
};
