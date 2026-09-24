// A requires-clause on a parameter of function type is accepted and silently
// dropped.
//
// A parameter declared with a function declarator is adjusted to a pointer to
// function ([dcl.fct]/5), so it declares no function and there is nothing for
// a requires-clause to constrain.  Every other misplaced requires-clause is
// diagnosed; this one is not.
//
// g++ -std=c++20 -fsyntax-only requires-clause-on-parameter.cpp
//
// Expected: rejected.  Actual: accepted with no diagnostic at all.

template <class T> concept C = true;

void f (int bar () requires true);

void g (int bar () requires true) { (void) bar; }

void h (int bar (int, int) requires true);

template <class T>
void t (T bar () requires C<T>);

struct S {
  void m (int bar () requires true);
};
