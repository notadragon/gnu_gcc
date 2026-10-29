// P3100: floating-point division by zero (ub:expr.mul.div.by.zero.fp.div)
// configured to "noexcept_enforce": the handler runs (kind=7, sem=7), then the
// program terminates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.fp.div:noexcept_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce reports then terminates" }

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
// { dg-output "VIOL kind=7 sem=7 comment=\\\[floating-point division by zero\\\]" }
