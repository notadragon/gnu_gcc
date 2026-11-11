// P3100: a call through a null pointer to member function (ub:expr.mptr.oper.member.func.null.function)
// configured to "observe": the handler runs (kind=7, sem=2) and execution continues.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <csetjmp>
namespace cs = std::contracts;
static std::jmp_buf env;
void handle_contract_violation (const cs::contract_violation& v) {
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
  std::longjmp (env, 1);	// do not continue into the undefined operation
}
struct S { int f () { return 1; } virtual int g () { return 2; } };
int (S::*volatile pmv) () = nullptr;
__attribute__((noinline)) int call (S &s, int (S::*pm) ()) { return (s.*pm) (); }
int main () {
  if (setjmp (env) == 0)
  {
    S s;
    std::printf ("RESULT=%d\n", call (s, pmv));
  }
  else
    std::printf ("RESUMED\n");

  return 0;
}
// { dg-output "VIOL kind=7 sem=2 comment=\\\[null pointer to member function\\\](\n|\r\n|\r)RESUMED" }
