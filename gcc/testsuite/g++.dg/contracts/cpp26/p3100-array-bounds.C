// P3100: array subscript out of bounds with a statically-known bound
// (ub:expr.add.out.of.bounds.known).  ignore -> the subscript is redirected to the
// valid index 0 (defined for both reads and writes; never actually out of
// bounds), no handler; observe -> handler runs (assertion_kind::implicit) then
// the access uses index 0.  Bounds is a front-end check, so a throwing observe
// is supported directly (no -fcontracts-p4298 needed).
// (Clang mirror: clang/test/Contracts/p3100-array-bounds.cpp)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-array-bounds.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace cs = std::contracts;

static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation (const cs::contract_violation& v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit)
    all_implicit = false;
}

int g[4] = { 10, 20, 30, 40 };
namespace ign_ns {                    // ignore
  int rd (int i) { return g[i]; }
  void wr (int i, int v) { g[i] = v; }
}
namespace obs_ns {                    // observe
  int rd (int i) { return g[i]; }
}

__attribute__((noinline)) int opaque (int x) { return x; }

int main () {
  // ignore: an out-of-bounds subscript is redirected to index 0, no handler.
  if (ign_ns::rd (opaque (100)) != 10) __builtin_abort ();   // g[0]
  if (ign_ns::rd (opaque (-1)) != 10) __builtin_abort ();    // negative -> g[0]
  if (ign_ns::rd (opaque (2)) != 30) __builtin_abort ();     // in bounds
  if (calls != 0) __builtin_abort ();

  ign_ns::wr (opaque (100), 99);                             // OOB write -> g[0]
  if (g[0] != 99) __builtin_abort ();
  g[0] = 10;

  // observe: handler runs, then the access uses index 0.
  if (obs_ns::rd (opaque (100)) != 10) __builtin_abort ();
  if (calls != 1) __builtin_abort ();
  if (!all_implicit) __builtin_abort ();
}
