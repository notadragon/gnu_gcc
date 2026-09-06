// A template's contracts are substituted when it is odr-used, not only when
// its definition is instantiated.
//
// GCC defers contract substitution to instantiate_body: tsubst_function_decl
// copies the pattern's specifiers onto an instantiation unsubstituted, and
// regenerate_decl_from_template does the real work when the definition is
// instantiated.  Two of our readers do not need a definition at all --
//
//   * P3097 evaluates a virtual function's interface contracts in a wrapper
//     around the vtable dispatch, and a PURE virtual has no definition to
//     instantiate;
//   * P3595 caller-side checking emits the check at the call site, so a
//     function template that is declared and called but never defined here has
//     its contracts read with no definition in sight.
//
// -- and each of them used to walk the still-dependent predicate straight into
// an ICE (`in check_noexcept_r, at cp/except.cc:1063`, or `in dependent_type_p,
// at cp/pt.cc:30282` when the predicate was type-dependent).  That was GCC-34,
// found 2026-09-05 by the pure-virtual case of
// contract-reentrancy-p3097-p3098.C.  [dcl.contract.func]/9 puts the odr-use
// and the definition on the same footing; maybe_instantiate_contracts is the
// odr-use half.
//
// `pre (true)` is not a regression test for this: a predicate that is already
// a constant needs no substitution to be usable, which is why the defect hid
// for as long as it did.  Every predicate below contains something
// substitution has to rewrite.
//
// Mirror of clang/test/Contracts/contract-instantiate-on-use.cpp, where all of
// this has always worked -- Clang substitutes contracts at every odr-use.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p3098" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/contract-instantiate-on-use.json" }

int f ();

// A pure virtual in a class template, predicate containing a call.

template <class T> struct PureCall {
  virtual void g () pre (f () == 0) = 0;
};

struct ConcreteCall : PureCall<int> {
  void g () override { }
};

void
use_pure_call (PureCall<int> &a)
{
  a.g ();
}

// The same, with a type-dependent predicate -- the shape that reached
// dependent_type_p rather than check_noexcept_r, so a fix that only quietened
// the noexcept walk would leave this one standing.

template <class T> struct PureDependent {
  virtual void g () pre (sizeof (T) == 4) = 0;
};

struct ConcreteDependent : PureDependent<int> {
  void g () override { }
};

void
use_pure_dependent (PureDependent<int> &a)
{
  a.g ();
}

// A pure virtual whose postcondition captures (P3098): the capture list has to
// substitute too, not just the predicate.

template <class T> struct PureCapturing {
  virtual int get () const post [old = f ()] (old == f ()) = 0;
};

struct ConcreteCapturing : PureCapturing<int> {
  int get () const override post [old = f ()] (old == f ()) { return 0; }
};

void
use_pure_capturing (PureCapturing<int> &a)
{
  a.get ();
}

// P3595 caller-side: a free function template that is declared, called, and
// never defined.  No virtual, no class.

template <class T> void declared_only () pre (f () == 0);

void
use_declared_only ()
{
  declared_only<int> ();
}

// The same for a member of a class template.

template <class T> struct Declared {
  void g () pre (f () == 0);
};

void
use_declared_member (Declared<int> &d)
{
  d.g ();
}

// Control: a predicate that is already a constant needs no substitution, and
// worked before the fix.  Kept so that a regression narrowing the fix to
// "only when dependent" still leaves this passing rather than silently
// changing which cases are covered.

template <class T> struct PureConstant {
  virtual void g () pre (true) = 0;
};

struct ConcreteConstant : PureConstant<int> {
  void g () override { }
};

void
use_pure_constant (PureConstant<int> &a)
{
  a.g ();
}

// Control: a virtual that IS defined.  Its contracts are substituted when the
// definition instantiates, so on-use instantiation must be a no-op rather than
// substituting a second time.

template <class T> struct Defined {
  virtual void g () pre (f () == 0) { }
};

void
use_defined (Defined<int> &d)
{
  d.g ();
}
