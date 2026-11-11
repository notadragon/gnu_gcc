// P3100: a null pointer to data member applied with .* (ub:expr.mptr.oper.member.func.null.data)
// configured to "noexcept_observe": the handler runs (kind=7, sem=6) and execution continues.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.data:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

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
// { dg-output "VIOL kind=7 sem=6 comment=\\\[null pointer to data member\\\](\n|\r\n|\r)RESULT=1" }
