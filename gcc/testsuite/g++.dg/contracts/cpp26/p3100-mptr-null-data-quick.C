// P3100: a null pointer to data member applied with .* (ub:expr.mptr.oper.member.func.null.data)
// configured to "quick_enforce": terminates without calling the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.data:quick_enforce" }
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
struct S { int a; int m; };
int S::*volatile pmv = nullptr;
__attribute__((noinline)) int *addr (S &s, int S::*pm) { return &(s.*pm); }
int main () {
  S s {};
  int *p = addr (s, pmv);
  std::printf ("RESULT=%d\n", p == (int *) ((char *) &s - 1));
  return 0;
}
