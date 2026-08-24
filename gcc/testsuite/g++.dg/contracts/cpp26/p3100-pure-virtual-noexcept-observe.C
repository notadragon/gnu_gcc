// P3100 x P4298: pure-virtual call (ub:class.abstract.pure.virtual) configured
// to the non-throwing "noexcept_observe" semantic.  The slot points at
// __cxa_pure_virtual_noexcept_observe: it reports through the handler (sem=6)
// and then terminates -- a pure-virtual call has no valid continuation.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-pure-virtual-noexcept-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "pure-virtual noexcept_observe reports then terminates" }

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

// { dg-output "VIOL kind=7 sem=6 comment=\\\[pure virtual function called\\\]" }
