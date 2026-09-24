// P3100: pure-virtual call (ub:class.abstract.pure.virtual) with a THROWING
// contract-violation handler under "enforce", where the pure virtual is NOT
// noexcept.  The __cxa_pure_virtual_enforce terminus lets the handler's
// exception unwind out through the (failed) construction to the caller's
// try/catch, so execution resumes there.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-pure-virtual-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

namespace cs = std::contracts;
struct Boom {};

void handle_contract_violation (const cs::contract_violation& v)
{
  std::printf ("VIOL sem=%d\n", (int) v.semantic ());
  std::fflush (stdout);
  throw Boom {};
}

struct Base
{
  Base () { poke (); }
  virtual void f () = 0;			// not noexcept: handler may propagate
  void poke ();
  virtual ~Base () {}
};
void Base::poke () { f (); }
struct Derived : Base { void f () override {} };

int main ()
{
  try { Derived d; (void) d; }
  catch (Boom&) { std::printf ("CAUGHT\n"); std::fflush (stdout); }
  std::printf ("SURVIVED\n");
  return 0;
}

// { dg-output "VIOL sem=3\r*\nCAUGHT\r*\nSURVIVED" }
