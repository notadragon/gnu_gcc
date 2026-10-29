// P3100: a member referred to in a constructor function-try-block handler (ub:except.handle.handler.ctor.dtor)
// a throwing violation handler under "enforce": the exception propagates to the caller.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:except.handle.handler.ctor.dtor:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL sem=%d\n", (int) v.semantic ()); std::fflush (stdout);
  throw Boom {};
}
static int *seen;
struct A {
  int x;
  int y = 42;
  A (int v) try : x (v == 1 ? throw 1 : 1) { }
  catch (int) { seen = &y; }	// y is referred to, not accessed
};
int main () {
  try {
  try { A a (1); } catch (int) { }
  std::printf ("RESULT=%d\n", seen != nullptr);
  }
  catch (Boom&) { std::printf ("CAUGHT\n"); std::fflush (stdout); }
  std::printf ("SURVIVED\n");
  return 0;
}
// { dg-output "VIOL sem=3(\n|\r\n|\r)CAUGHT(\n|\r\n|\r)SURVIVED" }
