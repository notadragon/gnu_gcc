// P3100: pure-virtual call (ub:class.abstract.pure.virtual) configured to
// "enforce".  The vtable slot points at __cxa_pure_virtual_enforce, which
// reports through the contract-violation handler (assertion_kind::implicit) and
// then terminates (the enforcing core aborts once the handler returns).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-pure-virtual-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "pure-virtual enforce reports then terminates" }

#include <contracts>
#include <cstdio>

namespace cs = std::contracts;

void handle_contract_violation (const cs::contract_violation& v)
{
  std::printf ("VIOL kind=%d sem=%d comment=[%s]\n",
	       (int) v.kind (), (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}

struct Base
{
  Base () { poke (); }
  virtual void f () = 0;
  void poke ();
  virtual ~Base () {}
};
void Base::poke () { f (); }
struct Derived : Base { void f () override {} };

int main () { Derived d; (void) d; return 0; }

// { dg-output "VIOL kind=7 sem=3 comment=\\\[pure virtual function called\\\]" }
