// [dcl.contract.func]/7 applied to an EXPLICIT OBJECT PARAMETER.
//
// An explicit object parameter ([dcl.fct]) is a parameter of the function, so
// the rule "if the predicate of a postcondition assertion of a function f
// odr-uses a non-reference parameter of f, that parameter ... shall have
// const type" governs it exactly as it governs any other by-value parameter.
// Nothing about deducing this was covered by the contracts testsuite before.
//
// The dependent cases matter as much as the plain ones: the rule is about a
// function, so a specialization whose parameter deduces to a non-const
// non-reference type is ill-formed even though the template itself is fine
// (a valid specialization exists, so [temp.res.general] does not make the
// template ill-formed NDR).  Clang currently misses every dependent shape.
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

// A by-value explicit object parameter is subject to the rule.
struct ByValue : S
{
  void f (this ByValue self) post (self.x == 0); // { dg-error "value parameter used in a postcondition must be const" }
};

// ... and satisfies it when declared const.
struct ByValueConst : S
{
  void f (this const ByValueConst self) post (self.x == 0);
};

// Reference explicit object parameters are exempt, in all three spellings.
struct ByRef : S
{
  void f (this ByRef &self) post (self.x == 0);
};

struct ByConstRef : S
{
  void f (this const ByConstRef &self) post (self.x == 0);
};

struct ByRvalueRef : S
{
  void f (this ByRvalueRef &&self) post (self.x == 0);
};

// The rule is about POSTconditions only.
struct InPrecondition : S
{
  void f (this InPrecondition self) pre (self.x == 0);
};

// ... and only about a parameter the predicate actually odr-uses.
struct NotNamed : S
{
  void f (this NotNamed self) post (true);
};

// With a result-name-introducer, the explicit object parameter is still
// caught.
struct WithResultName : S
{
  int f (this WithResultName self) post (r : r == self.x); // { dg-error "value parameter used in a postcondition must be const" }
};

// An ordinary parameter alongside a conforming explicit object parameter is
// still checked on its own account.
struct OrdinaryParameter : S
{
  void f (this const OrdinaryParameter self, int n) post (self.x == n); // { dg-error "value parameter used in a postcondition must be const" }
};

// A DEDUCED explicit object parameter, by value.  Deduction from a by-value
// parameter drops the argument's top-level cv-qualification, so Self deduces
// to the unqualified class type and the parameter is non-const -- ill-formed
// at instantiation, and ill-formed even when the object called on is const.
struct DeducedByValue : S
{
  template <class Self> void f (this Self self) post (self.x == 0); // { dg-error "value parameter .self. used in a postcondition must be const" }
};

void
use_deduced_by_value ()
{
  DeducedByValue a;
  a.f (); // { dg-message "required from here" }
}

// Calling on a const object deduces the SAME specialization -- deduction from
// a by-value parameter drops the argument's top-level cv-qualification -- so
// it is ill-formed for the same reason and emits no further diagnostic, the
// error above having already been given for this specialization.
void
use_deduced_by_value_on_const ()
{
  const DeducedByValue a;
  a.f ();
}

// The spelling that works: const on the parameter itself, so Self deduces to
// the unqualified type and the parameter is const.
struct DeducedByValueConst : S
{
  template <class Self> void f (this const Self self) post (self.x == 0);
};

void
use_deduced_by_value_const ()
{
  DeducedByValueConst a;
  a.f ();
}

// A deduced forwarding-reference explicit object parameter is exempt.
struct DeducedByRef : S
{
  template <class Self> void f (this Self &&self) post (self.x == 0);
};

void
use_deduced_by_ref ()
{
  DeducedByRef a;
  a.f ();
}

// Never instantiated: a valid specialization exists (Self = const T), so the
// template itself is well-formed and no diagnostic is required.
struct NeverInstantiated : S
{
  template <class Self> void f (this Self self) post (self.x == 0);
};

// An explicit object parameter of the enclosing class template's own type.
template <class T> struct InClassTemplate : S
{
  void f (this InClassTemplate self) post (self.x == 0); // { dg-error "value parameter .self. used in a postcondition must be const" }
};

void
use_in_class_template ()
{
  InClassTemplate<int> x; // { dg-message "required from here" }
  (void) x;
}
