// P3100 x P4298: implicit null-dereference (ub:expr.unary.dereference.nullptr)
// configured to the non-throwing "noexcept_enforce" semantic.  The middle end
// builds the contract_violation static data during the GIMPLE ubsan pass and
// calls the nothrow, noreturn handler entry point (no EH region needed): the
// handler runs, then the program terminates -- the real dereference is never
// reached.  dg-output confirms the handler ran with correctly populated static
// data (kind, semantic, comment, line); dg-shouldfail confirms termination.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-null-deref-noexcept-enforce.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce reports then terminates" }

#include <contracts>
#include <cstdio>

namespace cs = std::contracts;

extern const unsigned expected_line;

void handle_contract_violation (const cs::contract_violation& v)
{
  auto loc = v.location ();
  std::printf ("VIOL kind=%d sem=%d comment=[%s] line=%s\n",
	       (int) v.kind (), (int) v.semantic (), v.comment (),
	       loc.line () == expected_line ? "expected" : "unexpected");
  std::fflush (stdout);
  // returns -> the noreturn noexcept_enforce entry terminates; deref not reached
}

const unsigned expected_line = __LINE__ + 1;  // load_it's line below
int load_it (int *p) { return *p; }   // the dereference

int main ()
{
  int *p = nullptr;
  return load_it (p);
}

// { dg-output "VIOL kind=7 sem=7 comment=\\\[null pointer dereference\\\] line=expected" }
