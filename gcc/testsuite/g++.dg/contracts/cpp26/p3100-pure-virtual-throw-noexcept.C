// P3100: pure-virtual call with a THROWING handler under "enforce", where the
// pure virtual IS declared noexcept.  Because the pure virtual is noexcept, the
// compiler selects the terminate-on-throw terminus
// (__cxa_pure_virtual_noexcept_enforce, reported as sem=7), so a throwing
// handler must std::terminate rather than escaping into a caller that assumed
// the call could not throw -- even though the call site is inside a try/catch.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-pure-virtual-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "throwing handler at a noexcept pure virtual terminates" }

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
  virtual void f () noexcept = 0;		// noexcept: handler must terminate
  void poke ();
  virtual ~Base () {}
};
void Base::poke () { f (); }
struct Derived : Base { void f () noexcept override {} };

int main ()
{
  try { Derived d; (void) d; }
  catch (Boom&) { std::printf ("WRONGLY-CAUGHT\n"); std::fflush (stdout); }
  std::printf ("WRONGLY-SURVIVED\n");
  return 0;
}

// { dg-output "VIOL sem=7" }
