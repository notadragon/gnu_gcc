// P3100: a call through a null pointer to member function (ub:expr.mptr.oper.member.func.null.function)
// configured to "quick_enforce": terminates without calling the handler.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:quick_enforce" }
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
struct S { int f () { return 1; } virtual int g () { return 2; } };
int (S::*volatile pmv) () = nullptr;
__attribute__((noinline)) int call (S &s, int (S::*pm) ()) { return (s.*pm) (); }
int main () {
    S s;
    std::printf ("RESULT=%d\n", call (s, pmv));
  return 0;
}
