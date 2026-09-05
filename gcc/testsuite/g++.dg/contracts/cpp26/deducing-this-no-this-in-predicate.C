// `this` is unavailable inside an explicit object member function
// ([expr.prim.this]/1), and a contract predicate on such a function is inside
// it.  Naming `this` -- explicitly, or implicitly by naming a non-static data
// member unqualified, which means (*this).m -- must be rejected.
//
// The unqualified-member cases used to be reported as "'this' required when
// accessing a member within a constructor precondition or destructor
// postcondition contract check", for a function that is neither a constructor
// nor a destructor.  contract_class_ptr is set to current_class_ptr only for a
// constructor precondition or destructor postcondition and to NULL_TREE
// otherwise, and in an explicit object member function current_class_ptr is
// ALSO null, so the guard `contract_class_ptr == current_class_ptr` in
// cp/semantics.cc was true by coincidence.  Testing it for non-nullness lets
// the case fall through to the ordinary path, which gives it the same
// diagnostic the function BODY already gives -- consistency with the body
// being the point, rather than a new bespoke message.
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

// Explicit `this` in a POSTCONDITION, and in one with a
// result-name-introducer.  A postcondition is parsed on a different path from
// a precondition -- the result name is a declaration, and the predicate is
// dependent while it is being parsed -- so the two spellings above were not
// enough on their own.  Added by the audit of 2026-09-05, which found the
// explicit spelling covered only for `pre`.
struct ExplicitThisPost : S
{
  void f (this ExplicitThisPost &self) post (this->x == 0); // { dg-error "'this' is unavailable for explicit object member functions" }
};

struct ExplicitThisPostResult : S
{
  int f (this ExplicitThisPostResult &self) post (r : this->x == r); // { dg-error "'this' is unavailable for explicit object member functions" }
};

// And in an assertion-statement in the body, for the same reason the implicit
// spelling is covered there.
struct ExplicitThisAssert : S
{
  void
  f (this ExplicitThisAssert &self)
  {
    contract_assert (this->x == 0); // { dg-error "'this' is unavailable for explicit object member functions" }
  }
};

// An unqualified non-static data member in a precondition.
struct ImplicitThisPre : S
{
  void f (this ImplicitThisPre &self) pre (x == 0); // { dg-error "invalid use of non-static data member 'S::x'" }
};

// The same in a postcondition.
struct ImplicitThisPost : S
{
  int f (this ImplicitThisPost &self) post (r : x == r); // { dg-error "invalid use of non-static data member 'S::x'" }
};

// The same in an assertion-statement in the body.
struct ImplicitThisAssert : S
{
  void
  f (this ImplicitThisAssert &self)
  {
    contract_assert (x == 0); // { dg-error "invalid use of non-static data member 'S::x'" }
  }
};

// An unqualified MEMBER FUNCTION call in a predicate.  This is the second of
// the two guards in cp/semantics.cc, and it went wrong the same way.
struct ImplicitThisCall : S
{
  bool ok () const;
  void f (this ImplicitThisCall &self) pre (ok ()); // { dg-error "cannot call member function 'bool ImplicitThisCall::ok\\(\\) const' without object" }
};

// CONTROL: the explicit object parameter itself is of course usable, for a
// member function as well as for data.
struct ViaSelf : S
{
  bool ok () const;
  void f (this ViaSelf &self) pre (self.x == 0) pre (self.ok ());
};

// CONTROL: an ordinary (implicit object) member function may name `this` and
// its members in a predicate.
struct ImplicitObject : S
{
  void g () const pre (this->x == 0) pre (x == 0);
};
