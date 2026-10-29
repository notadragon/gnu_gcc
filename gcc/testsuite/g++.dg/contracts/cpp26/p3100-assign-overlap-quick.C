// P3100: a scalar assignment from an inexactly overlapping object (ub:expr.assign.overlap)
// configured to "quick_enforce": terminates without calling the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.assign.overlap:quick_enforce" }
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
union U { int i; short s; };
__attribute__((noinline)) int assign (U *u) { u->i = u->s; return u->i; }
int main () {
  U u;
  u.s = 1;
  int r = assign (&u);
  std::printf ("RESULT=%d\n", r == 1);
  return 0;
}
