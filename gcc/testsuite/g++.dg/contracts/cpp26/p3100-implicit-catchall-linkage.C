// P3100: a middle-end implicit check (here signed integer overflow) routed to a
// handler semantic via a bare "kind: implicit" catch-all configuration must
// link.  The shared contract descriptor table is finalized lazily by the first
// contract data block in the TU; under a catch-all it is first referenced by an
// inline <contracts> library function that is later reclaimed, and the
// descriptor was reclaimed with it -- leaving the middle-end check's own data
// block (built later in pass_ubsan) with a dangling descriptor reference and an
// undefined-symbol link error.  This exercises that end-to-end (build + run).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-implicit-catchall-linkage.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <climits>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation (const cs::contract_violation &v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit)
    __builtin_abort ();
}

// Signed overflow -> middle-end check -> noexcept_observe (via the catch-all,
// clamped from the base set under p4298): handler runs, continues wrapped.
int add (int a, int b) { return a + b; }

int main () {
  if (add (INT_MAX, 1) != INT_MIN) __builtin_abort ();   // overflow -> handler
  if (calls != 1) __builtin_abort ();
  if (add (2, 3) != 5) __builtin_abort ();               // no overflow, no call
  if (calls != 1) __builtin_abort ();
}
