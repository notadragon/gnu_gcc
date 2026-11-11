// P3100: a pure virtual function called directly in its class's constructor (ub:class.abstract.pure.virtual.direct)
// a throwing violation handler under "enforce": the exception propagates to the caller.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.direct:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-prune-output "called from constructor" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL sem=%d\n", (int) v.semantic ()); std::fflush (stdout);
  throw Boom {};
}
struct B {
  B () { f (); }		// resolved statically to B::f
  virtual void f () = 0;
  virtual ~B () {}
};
void B::f () { std::printf ("DEFINITION\n"); std::fflush (stdout); }
struct D : B { void f () override {} };
int main () {
  try {
  { D d; }
  std::printf ("RESULT=1\n");
  }
  catch (Boom&) { std::printf ("CAUGHT\n"); std::fflush (stdout); }
  std::printf ("SURVIVED\n");
  return 0;
}
// { dg-output "VIOL sem=3(\n|\r\n|\r)CAUGHT(\n|\r\n|\r)SURVIVED" }
