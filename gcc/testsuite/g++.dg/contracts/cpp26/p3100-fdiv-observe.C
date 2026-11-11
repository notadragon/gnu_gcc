// P3100: floating-point division by zero (ub:expr.mul.div.by.zero.fp.div)
// configured to "observe": the handler runs (kind=7, sem=2) and execution
// continues with the defined substitute.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.fp.div:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
__attribute__((noinline)) double div_it (double a, double b) { return a / b; }
int main () { std::printf ("RESULT=%g\n", div_it (1.0, 0.0)); return 0; }
// { dg-output "VIOL kind=7 sem=2 comment=\\\[floating-point division by zero\\\](\n|\r\n|\r)RESULT=0" }
