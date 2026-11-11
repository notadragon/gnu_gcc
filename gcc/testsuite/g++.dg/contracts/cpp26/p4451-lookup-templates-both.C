// p4451-lookup-templates.C under -fcontracts-p3400 and -fcontracts-p4451 (the P3400 label): each
// program is still rejected, by P3400's label checks (a parameter or
// variable is not an assertion-control object), which may diagnose each
// specialization.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p4451" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

template <class T> void f (T terminating) pre<terminating> (true);	// { dg-error "must be a class type" }
template <class T> struct S
{
  void g () pre<terminating> (true);	// { dg-error "must be a class type" }
  static constexpr int terminating = 0;
};
void use () { f (1); f (2.0); S<int> s; s.g (); S<long> t; t.g (); }

template <class T>
void h (T x)
{
  int terminating = 0;
  contract_assert<terminating> (x > 0);	// { dg-error "(must be a class type|not usable in a constant expression)" }
}
void use2 () { h (1); h (2L); }
