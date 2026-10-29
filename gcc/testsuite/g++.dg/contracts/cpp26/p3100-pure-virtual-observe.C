// P3100: a call that dispatches to a pure virtual function
// (ub:class.abstract.pure.virtual) configured to "observe".  The pure virtual's
// vtable slot is pointed at the __cxa_pure_virtual_observe terminus, which
// reports the violation through the contract-violation handler as an implicit
// (assertion_kind::implicit) assertion and then terminates -- a pure-virtual
// call has no valid continuation.  dg-output confirms the handler ran with the
// right kind/semantic/comment; dg-shouldfail confirms it then terminated.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-pure-virtual-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "pure-virtual observe reports then terminates" }

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
  Base () { poke (); }		// dispatches to the pure virtual during ctor
  virtual void f () = 0;
  void poke ();
  virtual ~Base () {}
};

// Out-of-line so the virtual call is a genuine vtable dispatch (no
// devirtualization): during Base's construction the vptr is Base's vtable, so
// f() resolves to the pure-virtual slot.
void Base::poke () { f (); }

struct Derived : Base { void f () override {} };

int main ()
{
  Derived d;
  (void) d;
  return 0;
}

// { dg-output "VIOL kind=7 sem=2 comment=\\\[pure virtual function called\\\]" }
