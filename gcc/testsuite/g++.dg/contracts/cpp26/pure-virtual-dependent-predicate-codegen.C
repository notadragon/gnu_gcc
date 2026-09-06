// A pure virtual in a class template whose contract predicate is
// value-dependent when parsed.  GCC has always compiled this; the file exists
// because Clang did not, and it is kept so both suites keep asking.
//
// It was CLANG-14 until 2026-09-06.  A pure virtual called by unqualified
// virtual dispatch is deliberately NOT odr-used ([basic.def.odr] excludes it
// from being "named by" the expression), and Clang hung contract
// instantiation off odr-use, so the predicate reached CodeGen still dependent.
// Nothing else would substitute it either -- a pure virtual has no definition
// to instantiate.
//
// [dcl.contract.func] makes contracts needed only when the function "is
// odr-used or the function is defined", so as written it never makes a pure
// virtual's contracts needed at all; a core issue is being filed to add a
// bullet matching [except.spec]'s "in an expression, the function is selected
// by overload resolution".  GCC reaches the same place from
// maybe_instantiate_contracts (GCC-34, GCC-35) rather than through odr-use,
// which is why it was never exposed here.
//
// THIS MUST STAY A CODEGEN TEST.  Clang's failure was invisible to
// -fsyntax-only, which is how a syntax-only probe matrix missed it.
//
// Mirror: clang/test/Contracts/pure-virtual-dependent-predicate-codegen.cpp

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097" }

int f ();

// The shape that crashes Clang: `n` is a member of a dependent class, so the
// predicate is value-dependent when parsed.

template <class T> struct A {
  virtual int get () const pre (n >= 0) = 0;
  int n;
};

struct D : A<int> {
  int get () const override { return n; }
};

int
use_dependent_member ()
{
  D d;
  A<int> &a = d;
  return a.get ();
}

// Also dependent, by way of the template parameter rather than a member.

template <class T> struct B {
  virtual int get () const pre (sizeof (T) == 4) = 0;
};

struct E : B<int> {
  int get () const override { return 0; }
};

int
use_dependent_param ()
{
  E e;
  B<int> &b = e;
  return b.get ();
}

// Controls: a non-dependent call, a non-pure virtual, and a non-template
// class.  All fine on both compilers, and they keep the Clang failure
// attributable to the dependence rather than to virtual contracts at large.

template <class T> struct NotDependent {
  virtual int get () const pre (f () == 0) = 0;
};
struct F1 : NotDependent<int> { int get () const override { return 0; } };

int
use_not_dependent ()
{
  F1 x;
  NotDependent<int> &r = x;
  return r.get ();
}

template <class T> struct NotPure {
  virtual int get () const pre (n >= 0) { return n; }
  int n;
};

int
use_not_pure ()
{
  NotPure<int> x{};
  return x.get ();
}

struct NotTemplate {
  virtual int get () const pre (n >= 0) = 0;
  int n;
};
struct F2 : NotTemplate { int get () const override { return n; } };

int
use_not_template ()
{
  F2 x{};
  NotTemplate &r = x;
  return r.get ();
}
