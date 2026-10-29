// P3100: a member referred to in a constructor function-try-block handler (ub:except.handle.handler.ctor.dtor)
// configured to "quick_enforce": terminates without calling the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:except.handle.handler.ctor.dtor:quick_enforce" }
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
static int *seen;
struct A {
  int x;
  int y = 42;
  A (int v) try : x (v == 1 ? throw 1 : 1) { }
  catch (int) { seen = &y; }	// y is referred to, not accessed
};
int main () {
  try { A a (1); } catch (int) { }
  std::printf ("RESULT=%d\n", seen != nullptr);
  return 0;
}
