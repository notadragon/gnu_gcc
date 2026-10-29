// P3100: a call through a null pointer to member function
// (ub:expr.mptr.oper.member.func.null.function) under "observe": a null
// pointer is caught with `.*' and `->*', for a polymorphic class and a
// non-polymorphic one; pointers to a non-virtual function, to the first
// virtual function and to a later one are not.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <csetjmp>
namespace cs = std::contracts;
static std::jmp_buf env;
static int viols;
void handle_contract_violation (const cs::contract_violation& v) {
  if (v.kind () == cs::assertion_kind::implicit)
    ++viols;
  std::longjmp (env, 1);	// do not call through the null pointer
}
struct S {
  int f () { return 1; }
  virtual int g () { return 2; }
  virtual int h () { return 3; }
};
struct N { int f () { return 4; } };
typedef int (S::*PS) ();
typedef int (N::*PN) ();
PS volatile snull = nullptr;
PN volatile nnull = nullptr;
__attribute__((noinline)) int dot (S &s, PS p) { return (s.*p) (); }
__attribute__((noinline)) int arrow (S *s, PS p) { return (s->*p) (); }
__attribute__((noinline)) int ndot (PN p) { N n; return (n.*p) (); }
int main () {
  S s;
  int r = dot (s, &S::f) + dot (s, &S::g) + arrow (&s, &S::h) + ndot (&N::f);
  std::printf ("SUM=%d viols=%d\n", r, viols);
  if (setjmp (env) == 0)
    dot (s, snull);
  if (setjmp (env) == 0)
    arrow (&s, snull);
  if (setjmp (env) == 0)
    ndot (nnull);
  std::printf ("viols=%d\n", viols);
  return 0;
}
// { dg-output "SUM=10 viols=0(\n|\r\n|\r)viols=3" }
