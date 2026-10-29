// P3100: a pure virtual function called directly in its class's constructor (ub:class.abstract.pure.virtual.direct)
// configured to "observe": the handler runs (kind=7, sem=2), then the program aborts (no valid continuation).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.direct:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "observe reports then aborts" }
// { dg-prune-output "called from constructor" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
struct B {
  B () { f (); }		// resolved statically to B::f
  virtual void f () = 0;
  virtual ~B () {}
};
void B::f () { std::printf ("DEFINITION\n"); std::fflush (stdout); }
struct D : B { void f () override {} };
int main () {
  { D d; }
  std::printf ("RESULT=1\n");
  return 0;
}
// { dg-output "VIOL kind=7 sem=2 comment=\\\[pure virtual function called\\\]" }
