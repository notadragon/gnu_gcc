// A requires-clause on a parameter of function type is ill-formed and must be
// diagnosed rather than silently dropped.
//
// A parameter declared with a function declarator is adjusted to a pointer to
// function ([dcl.fct]/5), so it declares no function and there is nothing for
// a requires-clause to constrain.  Every other misplaced requires-clause is
// already refused -- on a typedef, on a type-id, on a return type, on a
// declaration of non-function type -- and those are the controls at the
// bottom of this file.
//
// The parameter case escaped because the check that would have caught it,
// guarded on !FUNC_OR_METHOD_TYPE_P (type), runs BEFORE the
// function-to-pointer adjustment that makes its guard true: at that point the
// parameter still looks like a FUNCTION_TYPE.  Asking about decl_context
// rather than about the type is what closes it.
//
// Not a contracts bug, though it was found alongside one: the identical
// ordering lets a contract specifier through on the same declarator.  See
// g++.dg/contracts/cpp26/contract-on-non-function-declarator.C.
//
// { dg-do compile { target c++20 } }

template <class T> concept C = true;

void f (int bar () requires true);                  // { dg-error "requires-clause on parameter" }
void g (int bar () requires true) { (void) bar; }   // { dg-error "requires-clause on parameter" }
void h (int bar (int, int) requires true);          // { dg-error "requires-clause on parameter" }

template <class T>
void t (T bar () requires C<T>);                    // { dg-error "requires-clause on parameter" }

struct S {
  void m (int bar () requires true);                // { dg-error "requires-clause on parameter" }
};

// ---------------------------------------------------------------------------
// The sibling placements, already diagnosed, so that a change to one message
// does not quietly diverge from the others.
// ---------------------------------------------------------------------------

typedef int Tdef (int) requires true;               // { dg-error "requires-clause on typedef" }
using Alias = int (int) requires true;              // { dg-error "requires-clause on type-id" }
int (*ptr) (int) requires true;                     // { dg-error "requires-clause on declaration of non-function type" }
int (*ret (int)) (int) requires true;               // { dg-error "requires-clause on return type" }

// ---------------------------------------------------------------------------
// Controls: a requires-clause where one belongs.
// ---------------------------------------------------------------------------

template <class T> void ok (T v) requires C<T>;

template <class T> struct Holder { void m (T v) requires C<T>; };

void plain (int (*p) (int));

auto lam = [] <class T> (T v) requires C<T> { return v; };
