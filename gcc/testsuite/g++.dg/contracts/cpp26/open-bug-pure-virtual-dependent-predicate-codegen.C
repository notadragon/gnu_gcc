// Mirror of a CLANG-ONLY open bug: a pure virtual in a class template whose
// contract predicate is value-dependent when parsed.
//
// Tracked as CLANG-14 in the llvm_llvm-project fork's open-issues/.  There the
// predicate reaches CodeGen still dependent and the constant evaluator asserts
// (`!isValueDependent()`, ExprConstant.cpp).  All three of pure, class
// template, and a value-dependent predicate are required; a non-dependent call
// such as `pre (f () == 0)` compiles clean on Clang.
//
// Measured 2026-09-05: GCC compiles this, and a runnable version confirms the
// interface precondition is genuinely evaluated through the P3097 wrapper (one
// violation for a failing value, none for a passing one).  So this file is NOT
// xfailed here; it is a plain expected-pass, and its job is to keep the two
// suites asking the same question.
//
// GCC only reached that state earlier the same day: before GCC-34 was fixed it
// ICEd on an overlapping set of shapes.  The two compilers were broken here in
// different halves, and this intersection -- pure + class template + dependent
// predicate -- is what neither suite had a test for.
//
// NOTE: this must be a codegen test, not `-fsyntax-only`.  Clang's failure is
// in CodeGen, and the whole syntax-only probe matrix behind CLANG-13 walked
// past it.
//
// Mirror: clang/test/Contracts/OpenBugs/pure-virtual-dependent-predicate-codegen.cpp

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
