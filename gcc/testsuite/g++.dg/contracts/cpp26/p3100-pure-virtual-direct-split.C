// P3100: configuring only the .indirect leaf of ub:class.abstract.pure.virtual
// does not check a call resolved statically in the class's own constructor
// (the .direct leaf): the pure virtual's definition runs, as without the
// check.  Configuring the parent group checks it.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.indirect:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-prune-output "called from constructor" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d\n", (int) v.kind (), (int) v.semantic ());
  std::fflush (stdout);
}
struct B {
  B () { f (); }
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
// { dg-output "^DEFINITION(\n|\r\n|\r)RESULT=1" }
