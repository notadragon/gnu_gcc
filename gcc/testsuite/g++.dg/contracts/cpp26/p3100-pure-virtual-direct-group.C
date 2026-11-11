// P3100: configuring the parent group ub:class.abstract.pure.virtual checks a
// call resolved statically in the class's own destructor (the .direct leaf):
// noexcept_observe reports (kind=7, sem=6) and then aborts.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_observe reports then aborts" }
// { dg-prune-output "called from destructor" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
struct B {
  virtual void f () = 0;
  virtual ~B () { f (); }
};
void B::f () { std::printf ("DEFINITION\n"); std::fflush (stdout); }
struct D : B { void f () override {} };
int main () {
  { D d; }
  std::printf ("RESULT=1\n");
  return 0;
}
// { dg-output "^VIOL kind=7 sem=6 comment=\\\[pure virtual function called\\\]" }
