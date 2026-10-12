// P3595 caller-side checking (caller observe, callee ignore) checks calls to
// a constructor, whose call names a clone that has no contracts of its own.
// A member-function call is the control; all four calls below must be
// reported.
//
// No Clang mirror: Clang has no caller-side checking.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-side-ctor.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;

void handle_contract_violation (const std::contracts::contract_violation&)
{
  ++violations;
}

struct A {
  int v;
  A (int x) pre (x > 0) : v (x) {}
  A (const char*, int y) pre (y > 0) : v (y) {}
  int get (int d) const pre (d > 0) { return v; }
};

int main ()
{
  A a (-1);
  A b ("s", -1);
  A* p = new A (-2);
  a.get (-1);
  delete p;
  if (violations != 4)
    __builtin_abort ();
}
