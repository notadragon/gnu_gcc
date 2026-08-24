// P3100: pure-virtual call (ub:class.abstract.pure.virtual) configured to
// "quick_enforce".  The vtable slot points at __cxa_pure_virtual_quick, a fast
// silent trap -- no handler is called and nothing is printed.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-pure-virtual-quick.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "pure-virtual quick_enforce traps" }

#include <contracts>
#include <cstdio>

namespace cs = std::contracts;

// Must NOT be called under quick_enforce.
void handle_contract_violation (const cs::contract_violation&)
{
  std::printf ("HANDLER-SHOULD-NOT-RUN\n");
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

// quick_enforce traps during construction before any handler runs; the codegen
// test (p3100-pure-virtual-codegen.C) confirms the slot targets
// __cxa_pure_virtual_quick rather than a handler-calling terminus.
