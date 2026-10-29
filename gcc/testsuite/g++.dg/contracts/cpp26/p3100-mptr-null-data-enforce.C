// P3100: a null pointer to data member applied with .* (ub:expr.mptr.oper.member.func.null.data)
// configured to "enforce": the handler runs (kind=7, sem=3), then the program terminates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.data:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "enforce reports then terminates" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
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
// { dg-output "VIOL kind=7 sem=3 comment=\\\[null pointer to data member\\\]" }
