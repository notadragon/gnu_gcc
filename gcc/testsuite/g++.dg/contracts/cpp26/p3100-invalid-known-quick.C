// P3100: an indirection past the end of an array of known bound
// (ub:expr.unary.dereference.invalid.known) configured to "quick_enforce":
// terminates without calling the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:quick_enforce" }
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
int arr[3] = {10, 20, 30};
__attribute__((noinline)) int *addr (int i) { return &arr[i]; }
int main () { std::printf ("RESULT=%d\n", (int) (addr (3) - arr)); return 0; }
