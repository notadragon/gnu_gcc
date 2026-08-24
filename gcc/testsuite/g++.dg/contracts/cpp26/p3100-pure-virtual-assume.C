// P3100: pure-virtual call (ub:class.abstract.pure.virtual) configured to
// "assume" keeps the legacy behavior -- the vtable slot stays the plain
// __cxa_pure_virtual (which prints "pure virtual method called" and terminates),
// NOT a contract-aware terminus.  A pure-virtual call has no value to
// substitute, so "ignore" behaves the same; this test pins the assume/ignore
// status quo so the contract wiring does not perturb it.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-pure-virtual-assume.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "legacy __cxa_pure_virtual terminates" }

#include <contracts>
#include <cstdio>

namespace cs = std::contracts;

// Must NOT be called: assume keeps the legacy terminus.
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

// { dg-output "pure virtual method called" }
