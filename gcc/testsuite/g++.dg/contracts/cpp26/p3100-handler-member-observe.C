// P3100: a member referred to in a constructor function-try-block handler (ub:except.handle.handler.ctor.dtor)
// configured to "observe": the handler runs (kind=7, sem=2) and execution continues.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:except.handle.handler.ctor.dtor:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
static int *seen;
struct A {
  int x;
  int y = 42;
  A (int v) try : x (v == 1 ? throw 1 : 1) { }
  catch (int) { seen = &y; }	// y is referred to, not accessed
};
int main () {
  try { A a (1); } catch (int) { }
  std::printf ("RESULT=%d\n", seen != nullptr);
  return 0;
}
// { dg-output "VIOL kind=7 sem=2 comment=\\\[member referred to in a constructor or destructor function-try-block handler\\\](\n|\r\n|\r)RESULT=1" }
