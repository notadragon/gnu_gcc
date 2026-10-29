// P3100: a pure virtual function called directly in its class's constructor (ub:class.abstract.pure.virtual.direct)
// configured to "quick_enforce": terminates without calling the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.direct:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "quick_enforce terminates" }
// { dg-prune-output "called from constructor" }

#include <contracts>
#include <cstdio>
#include <cstdlib>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  /* quick_enforce never calls the handler: a call is a failure, and
     exiting normally is how a dg-shouldfail test reports one.  */
  (void) v;
  std::_Exit (0);
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
