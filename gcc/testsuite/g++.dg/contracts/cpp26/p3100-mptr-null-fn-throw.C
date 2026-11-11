// P3100: a call through a null pointer to member function (ub:expr.mptr.oper.member.func.null.function)
// a throwing violation handler under "enforce": the exception propagates to the caller.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL sem=%d\n", (int) v.semantic ()); std::fflush (stdout);
  throw Boom {};
}
struct S { int f () { return 1; } virtual int g () { return 2; } };
int (S::*volatile pmv) () = nullptr;
__attribute__((noinline)) int call (S &s, int (S::*pm) ()) { return (s.*pm) (); }
int main () {
  try {
    S s;
    std::printf ("RESULT=%d\n", call (s, pmv));
  }
  catch (Boom&) { std::printf ("CAUGHT\n"); std::fflush (stdout); }
  std::printf ("SURVIVED\n");
  return 0;
}
// { dg-output "VIOL sem=3(\n|\r\n|\r)CAUGHT(\n|\r\n|\r)SURVIVED" }
