// Detectors D2 and D3 of the requires-clause declarator matrix (D1, does the
// declaration compile, is concepts-requires-declarator.C).
//
// D2: an unsatisfiable clause, then a call.  Honoured, the call finds no
// viable function; dropped, the call succeeds.
// D3: two overloads with complementary clauses and different return types,
// each call selecting one.  Honoured, both coexist and each initialization
// below is well-formed; a clause dropped or attached to the wrong declarator
// makes the calls ambiguous (or the second declaration a redefinition).
// Only D3 shows that the clause belongs to the function the grammar assigns
// it to.
//
// The requires-clause after a declarator whose return type is a pointer or
// reference to function, or a pointer to array, belongs to the declared
// function ([dcl.decl.general]).  GCC rejects those declarations (upstream
// PR123909, PR94984), so the Group B rows are xfail watch tests: each line
// GCC currently diagnoses carries an xfailed dg-bogus, and each expected
// "no matching function" an xfailed dg-error.  The controls C1-C3 must pass.
//
// Mirror: clang/test/CXX/dcl.decl/dcl.decl.general/requires-declarator-effect.cpp in the
// llvm_llvm-project fork, which gets every row right.
// { dg-do compile { target c++20 } }

template <class T> concept Small = sizeof (T) <= 4;
template <class T> concept Never = false;

int fi (int);
long fl (int);
char arr[3];
using PFi = int (*) (int);
using PFl = long (*) (int);
using PA = char (*) [3];

// ---------------------------------------------------------------------------
// D2
// ---------------------------------------------------------------------------

template <class T> int (*n1 (T)) (int) requires Never<T>;	// { dg-bogus "requires-clause" "PR123909" { xfail *-*-* } }
template <class T> auto n4 (T) -> int (*) (int) requires Never<T>; // { dg-bogus "requires-clause" "PR123909" { xfail *-*-* } }
template <class T> char (*n6 (T)) [3] requires Never<T>;	// { dg-bogus "expected initializer" "PR94984" { xfail *-*-* } }
// Controls.
template <class T> int nc1 (T) requires Never<T>;
template <class T> auto nc2 (T) -> int requires Never<T>;
template <class T> auto nc3 (T) -> char (*) [3] requires Never<T>;

void
d2 ()
{
  n1 (0);	// { dg-error "no matching function" "PR123909" { xfail *-*-* } }
  n4 (0);	// { dg-error "no matching function" "PR123909" { xfail *-*-* } }
  n6 (0);	// { dg-error "no matching function" "PR94984" { xfail *-*-* } }
  // { dg-bogus "not declared" "PR94984" { xfail *-*-* } .-1 }
  nc1 (0);	// { dg-error "no matching function" }
  nc2 (0);	// { dg-error "no matching function" }
  nc3 (0);	// { dg-error "no matching function" }
}

// ---------------------------------------------------------------------------
// D3
// ---------------------------------------------------------------------------

template <class T> int (*p1 (T)) (int) requires Small<T> { return fi; }	// { dg-bogus "requires-clause" "PR123909" { xfail *-*-* } }
template <class T> long (*p1 (T)) (int) requires (!Small<T>) { return fl; } // { dg-bogus "requires-clause" "PR123909" { xfail *-*-* } }
PFi r1a = p1 (0);			// { dg-bogus "ambiguous" "PR123909" { xfail *-*-* } }
PFl r1b = p1 (0LL);			// { dg-bogus "ambiguous" "PR123909" { xfail *-*-* } }

template <class T> auto p4 (T) -> int (*) (int) requires Small<T> { return fi; } // { dg-bogus "requires-clause" "PR123909" { xfail *-*-* } }
template <class T> auto p4 (T) -> long (*) (int) requires (!Small<T>) { return fl; } // { dg-bogus "requires-clause" "PR123909" { xfail *-*-* } }
PFi r4a = p4 (0);			// { dg-bogus "ambiguous" "PR123909" { xfail *-*-* } }
PFl r4b = p4 (0LL);			// { dg-bogus "ambiguous" "PR123909" { xfail *-*-* } }

// Same return type both times: a dropped clause makes the second a
// redefinition of the first.
template <class T> char (*p6 (T)) [3] requires Small<T> { return &arr; }	// { dg-bogus "expected initializer" "PR94984" { xfail *-*-* } }
template <class T> char (*p6 (T)) [3] requires (!Small<T>) { return &arr; } // { dg-bogus "expected initializer" "PR94984" { xfail *-*-* } }
PA r6a = p6 (0);			// { dg-bogus "not declared" "PR94984" { xfail *-*-* } }
PA r6b = p6 (0LL);			// { dg-bogus "not declared" "PR94984" { xfail *-*-* } }

// Controls.
template <class T> int c1 (T) requires Small<T> { return 1; }
template <class T> long c1 (T) requires (!Small<T>) { return 2; }
static_assert (__is_same (decltype (c1 (0)), int));
static_assert (__is_same (decltype (c1 (0LL)), long));

template <class T> auto c2 (T) -> int requires Small<T> { return 1; }
template <class T> auto c2 (T) -> long requires (!Small<T>) { return 2; }
static_assert (__is_same (decltype (c2 (0)), int));
static_assert (__is_same (decltype (c2 (0LL)), long));

template <class T> auto c3 (T) -> char (*) [3] requires Small<T> { return &arr; }
template <class T> auto c3 (T) -> char (*) [3] requires (!Small<T>) { return &arr; }
PA rc3a = c3 (0);
PA rc3b = c3 (0LL);
