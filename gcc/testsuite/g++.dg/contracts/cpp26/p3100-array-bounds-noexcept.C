// P3100 x P4298: array subscript out of bounds configured to noexcept_observe.
// The nothrow handler runs with a populated contract_violation (kind=implicit,
// the right semantic, comment), then the access uses index 0.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-array-bounds-noexcept.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation (const cs::contract_violation& v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit) __builtin_abort ();
  if (v.semantic () != cs::evaluation_semantic::noexcept_observe)
    __builtin_abort ();
  if (__builtin_strcmp (v.comment (), "array subscript out of bounds") != 0)
    __builtin_abort ();
}

int g[4] = { 10, 20, 30, 40 };
__attribute__((noinline)) int rd (int i) { return g[i]; }

int main () {
  int r = rd (100);            // handler runs (nothrow), then index 0
  if (calls != 1) __builtin_abort ();
  if (r != 10) __builtin_abort ();   // g[0]
}
