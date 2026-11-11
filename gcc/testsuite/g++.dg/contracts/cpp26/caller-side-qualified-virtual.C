// A qualified call to a virtual function under P3595 caller-side checking
// (caller observe, callee ignore) calls the named function directly and
// checks its contract caller-side: its wrapper is keyed by the call's caller
// tuple, not treated as a virtual-dispatch wrapper.
//
// No Clang mirror: Clang has no caller-side checking.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/open-bug-caller-side.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0, b_calls = 0, d_calls = 0;

void handle_contract_violation (const std::contracts::contract_violation&)
{
  ++violations;
}

struct B {
  virtual void f (int x) pre (x > 0) { ++b_calls; }
  virtual ~B () {}
};
struct D : B {
  void f (int) override { ++d_calls; }
};

int main ()
{
  D d;
  d.B::f (-1);
  if (violations != 1 || b_calls != 1 || d_calls != 0)
    __builtin_abort ();
}
