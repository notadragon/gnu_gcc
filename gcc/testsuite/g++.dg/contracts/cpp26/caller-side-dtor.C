// Under P3595 caller-side checking (caller observe, callee ignore), a call to
// a destructor is checked like any other call: the call names a destructor
// clone, whose contracts are its origin's.  A member-function call is the
// control; both calls below must be reported.
//
// No Clang mirror: Clang has no caller-side checking.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/open-bug-caller-side.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;

void handle_contract_violation (const std::contracts::contract_violation&)
{
  ++violations;
}

struct D { int k = -1; ~D () pre (k > 0) {} };
struct E { int k = -1; void f () pre (k > 0) {} };

int main ()
{
  { D d; }
  E e;
  e.f ();
  if (violations != 2)
    __builtin_abort ();
}
