// P3100: a null pointer passed to a non-allocating placement new (ub:expr.new.non.allocating.null)
// configured to "quick_enforce": terminates without calling the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.new.non.allocating.null:quick_enforce" }
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
struct T { int x; };	// trivial: `new (p) T' stores nothing
void *volatile nowhere = nullptr;
__attribute__((noinline)) T *make (void *p) { return new (p) T; }
int main () {
  T *t = make (nowhere);
  std::printf ("RESULT=%d\n", t == nullptr);
  return 0;
}
