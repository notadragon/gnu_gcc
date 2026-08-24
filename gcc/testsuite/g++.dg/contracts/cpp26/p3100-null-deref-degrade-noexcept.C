// P3100 x P4298: GCC does not yet support the potentially-throwing
// enforce/observe for the middle-end null-dereference check (the site runs
// after EH lowering).  Rather than diagnosing a configured "observe", the
// null-deref allowed set excludes throwing enforce/observe, so resolution
// clamps it -- and with -fcontracts-p4298 the fallback order degrades "observe"
// to its nothrow counterpart "noexcept_observe" (and "enforce" to
// "noexcept_enforce").  The handler here confirms it was invoked with the
// degraded noexcept_observe semantic.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0 -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-null-deref-degrade-noexcept.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>

namespace cs = std::contracts;

void handle_contract_violation (const cs::contract_violation& v)
{
  if (v.kind () != cs::assertion_kind::implicit)
    std::exit (2);
  // "observe" was configured but is not supported here; it degrades to the
  // nothrow noexcept_observe.
  if (v.semantic () != cs::evaluation_semantic::noexcept_observe)
    std::exit (3);
  std::exit (0);   // proves the (degraded) handler ran
}

int load_it (int *p) { return *p; }

int main ()
{
  int *p = nullptr;
  load_it (p);
  std::abort ();
}
