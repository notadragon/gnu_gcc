// P3100: a scalar assignment from an inexactly overlapping object (ub:expr.assign.overlap)
// configured to "ignore": no handler call, and execution continues.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.assign.overlap:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
union U { int i; short s; };
__attribute__((noinline)) int assign (U *u) { u->i = u->s; return u->i; }
int main () {
  U u;
  u.s = 1;
  int r = assign (&u);
  std::printf ("RESULT=%d\n", r == 1);
  return 0;
}
// { dg-output "^RESULT=1" }
