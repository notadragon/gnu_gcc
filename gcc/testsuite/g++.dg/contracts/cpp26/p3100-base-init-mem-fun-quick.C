// P3100: a member function called in a base-class mem-initializer (ub:class.base.init.mem.fun.nonpolymorphic.mem.init)
// configured to "quick_enforce": terminates without calling the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:class.base.init.mem.fun.nonpolymorphic.mem.init:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "quick_enforce terminates" }

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
struct Base { int v; Base (int y) : v (y) { } };
struct Derived : Base {
  int j;
  int f () const { return 7; }	// reads no member
  Derived () : Base (f ()), j (5) { }
};
int main () {
  Derived d;
  std::printf ("RESULT=%d\n", d.v == 7);
  return 0;
}
