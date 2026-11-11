// P3100: a misaligned placement new that stores nothing (ub:basic.align.object.alignment.nostore)
// configured to "quick_enforce": terminates without calling the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:quick_enforce" }
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
#include <new>
alignas (int) char buf[4 * sizeof (int)];
void *volatile where = buf + 1;
__attribute__((noinline)) int *make (void *p) { return new (p) int; }
int main () {
  int *q = make (where);
  std::printf ("RESULT=%d\n", (void *) q == where);
  return 0;
}
