// P3100 x P4298: implicit null-dereference (ub:expr.unary.dereference.nullptr)
// configured to the non-throwing "noexcept_observe" semantic.  The middle end
// builds the contract_violation static data during the GIMPLE ubsan pass and
// calls the nothrow handler entry point (no EH region needed).  For null-deref
// there is no defined lvalue to substitute, so "observe" reports and then
// PROCEEDS into the real (still-UB) dereference -- the program crashes after
// the handler returns.  The dg-output confirms the handler ran with correctly
// populated static data (kind, semantic, comment, line); dg-shouldfail confirms
// it then proceeded into the dereference.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0 -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-null-deref-noexcept-observe.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_observe reports then proceeds into the null deref" }

#include <contracts>
#include <cstdio>

namespace cs = std::contracts;

void handle_contract_violation (const cs::contract_violation& v)
{
  auto loc = v.location ();
  std::printf ("VIOL kind=%d sem=%d comment=[%s] line=%u\n",
	       (int) v.kind (), (int) v.semantic (), v.comment (),
	       (unsigned) loc.line ());
  std::fflush (stdout);
  // returns -> noexcept_observe proceeds into the real (null) dereference
}

int load_it (int *p) { return *p; }   // line 31: the dereference

int main ()
{
  int *p = nullptr;
  return load_it (p);
}

// { dg-output "VIOL kind=7 sem=6 comment=\\\[null pointer dereference\\\] line=31" }
