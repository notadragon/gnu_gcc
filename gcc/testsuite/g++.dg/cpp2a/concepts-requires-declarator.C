// The requires-clause of an init-declarator or member-declarator belongs to
// the declarator as a whole, which must declare a templated function
// ([dcl.decl.general]); it is never part of a nested declarator or of a
// type-id.  So the clause is ill-formed exactly where no function is
// declared (Group A), and well-formed on every declarator that declares a
// function, however its return type is spelled (Group B).
//
// This file is detector D1 of the matrix: does the declaration compile at
// all.  concepts-requires-declarator-effect.C holds D2 (is the clause
// evaluated) and D3 (is it attached to the right function).
//
// Group B is wrongly rejected by GCC: the parser attaches the clause to the
// nested declarator whose parameter list precedes it, or does not reach it
// at all after an array declarator.  Upstream PR123909 (pointer and
// reference to function) and PR94984 (array declarator); the rows are xfail
// watch tests, so an upstream fix shows up as XPASS.  Group A is
// requires-clause-on-parameter.C's sibling list, repeated here so that the
// whole rule is stated in one place.
//
// Mirror: clang/test/CXX/dcl.decl/dcl.decl.general/requires-declarator.cpp in the
// llvm_llvm-project fork, which accepts Group B.
// { dg-do compile { target c++20 } }

template <class T> concept C = sizeof (T) > 0;
struct S { int m (int); };

// ---------------------------------------------------------------------------
// Group A -- no function is declared: the clause is refused.
// ---------------------------------------------------------------------------

template <class T> struct A
{
  typedef int F (int) requires C<T>;			// { dg-error "requires-clause on typedef" }
  using G = int (int) requires C<T>;			// { dg-error "requires-clause on type-id" }
  int (*p) (int) requires C<T>;				// { dg-error "requires-clause on declaration of non-function type" }
  void q (int bar () requires C<T>);			// { dg-error "requires-clause on parameter" }
};

// ---------------------------------------------------------------------------
// Group B -- a function is declared: the clause is well-formed.
// ---------------------------------------------------------------------------

// B1/B2: returns a pointer to function, classic spelling.
template <class T> int (*b1 (T i)) (int) requires C<T>;	// { dg-bogus "requires-clause" "PR123909" { xfail *-*-* } }
// B3: returns a pointer to member function.
template <class T> int (S::*b3 (T i)) (int) requires C<T>; // { dg-bogus "requires-clause" "PR123909" { xfail *-*-* } }
// B4: trailing return type naming a pointer to function.
template <class T> auto b4 (T i) -> int (*) (int) requires C<T>; // { dg-bogus "requires-clause" "PR123909" { xfail *-*-* } }
// B5: trailing return type naming a reference to function.
template <class T> auto b5 (T i) -> int (&) (int) requires C<T>; // { dg-bogus "requires-clause" "PR123909" { xfail *-*-* } }
// B6: returns a pointer to array.
template <class T> char (*b6 (T i)) [3] requires C<T>;	// { dg-bogus "expected initializer" "PR94984" { xfail *-*-* } }
// B7: returns a reference to array.
template <class T> char (&b7 (T i)) [3] requires C<T>;	// { dg-bogus "expected initializer" "PR94984" { xfail *-*-* } }

// The same shapes as members of a class template.
template <class T> struct M
{
  int (*b1 (int)) (int) requires C<T>;			// { dg-bogus "requires-clause" "PR123909" { xfail *-*-* } }
  char (*b6 (int)) [3] requires C<T>;			// { dg-bogus "expected" "PR94984" { xfail *-*-* } }
};

// ---------------------------------------------------------------------------
// Controls -- the shapes that work, so that a change for Group B that stops
// honouring an ordinary constrained template is caught here.
// ---------------------------------------------------------------------------

// C1: the function declarator is the outermost one.
template <class T> int c1 (T i) requires C<T>;
// C2: a trailing return type with no parameter list in its type-id.
template <class T> auto c2 (T i) -> int requires C<T>;
// C3: B4 minus the parameter list: a trailing pointer-to-array type-id.
template <class T> auto c3 (T i) -> char (*) [3] requires C<T>;
template <class T> struct N
{
  int c1 (int) requires C<T>;
  auto c3 (int) -> char (*) [3] requires C<T>;
};
