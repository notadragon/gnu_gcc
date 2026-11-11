// P3100: a noexcept pure virtual function called directly in its class's
// constructor (ub:class.abstract.pure.virtual.direct) under "enforce" with a
// throwing handler: as at the vtable terminus, the semantic is promoted to
// noexcept_enforce (sem=7), so the exception terminates the program instead
// of escaping from a call that cannot throw.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual.direct:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce terminates" }
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
  B () { f (); }
  virtual void f () noexcept = 0;
  virtual ~B () {}
};
void B::f () noexcept { }
struct D : B { void f () noexcept override {} };
int main () {
  try { D d; }
  catch (Boom &) { std::printf ("CAUGHT\n"); }
  return 0;
}
// { dg-output "^VIOL sem=7" }
