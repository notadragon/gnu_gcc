// P3100: a [[noreturn]] function that returns (ub:dcl.attr.noreturn.eventually.returns)
// configured to "quick_enforce": terminates without calling the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:quick_enforce" }
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
// { dg-prune-output "does return" }
[[noreturn]] __attribute__((noinline)) void nr (int x) { if (x < 0) throw x; }
void (*volatile fp) (int) = nr;	// the caller does not know nr is noreturn
int main () {
  fp (1);
  std::printf ("RESULT=1\n");
  return 0;
}
