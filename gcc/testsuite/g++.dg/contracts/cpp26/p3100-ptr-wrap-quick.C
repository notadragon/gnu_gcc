// P3100: pointer arithmetic that wraps around the address space
// (ub:expr.add.out.of.bounds.pointer.arithmetic) configured to
// "quick_enforce": terminates without calling the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:quick_enforce" }
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
#include <cstdint>
__attribute__((noinline)) char *add (char *p, long i) { return p + i; }
char *top = reinterpret_cast<char *> (UINTPTR_MAX - 1);
int main () { std::printf ("RESULT=%d\n", (int) (add (top, 4) - top)); return 0; }
