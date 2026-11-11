// P4451 [basic.contract.control]: the lookup check of `<terminating>' in
// templates.  A function template's parameter, a class template's member
// declared later and a block-scope variable in a function template are
// each diagnosed once, however many specializations are instantiated (one
// dg-regexp per message, so a second one is an excess error).  Under
// -fcontracts-p4451 alone.
//
// Mirror: tests/tests/contracts/p4451/lookup_templates.sft.cpp in the
// edgcpp_compiler fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

template <class T> void f (T terminating) pre<terminating> (true);
template <class T> struct S
{
  void g () pre<terminating> (true);
  static constexpr int terminating = 0;
};
void use () { f (1); f (2.0); S<int> s; s.g (); S<long> t; t.g (); }

template <class T>
void h (T x)
{
  int terminating = 0;
  contract_assert<terminating> (x > 0);
}
void use2 () { h (1); h (2L); }

// { dg-regexp {[^\n]*p4451-lookup-templates\.C:16:[0-9]+: error: unqualified lookup of .terminating. from this assertion-control-specifier finds[^\n]*\n} }
// { dg-regexp {[^\n]*p4451-lookup-templates\.C:19:[0-9]+: error: unqualified lookup of .terminating. from this assertion-control-specifier finds[^\n]*\n} }
// { dg-regexp {[^\n]*p4451-lookup-templates\.C:28:[0-9]+: error: unqualified lookup of .terminating. from this assertion-control-specifier finds[^\n]*\n} }
