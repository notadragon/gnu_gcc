// P3100 x P4298: invalid bool value load configured to noexcept_observe.  The
// nothrow handler runs with a populated contract_violation (kind=implicit, the
// right semantic, comment), then execution continues with the defined value.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-invalid-value-noexcept.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation (const cs::contract_violation& v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit) __builtin_abort ();
  if (v.semantic () != cs::evaluation_semantic::noexcept_observe)
    __builtin_abort ();
  if (__builtin_strcmp (v.comment (), "invalid value for its type") != 0)
    __builtin_abort ();
}

__attribute__((noinline)) bool load (const bool* p) { return *p; }

int main () {
  unsigned char c = 4;
  bool b; __builtin_memcpy (&b, &c, 1);
  bool r = load (&b);          // handler runs (nothrow), then continues
  if (calls != 1) __builtin_abort ();
  if (r != false) __builtin_abort ();   // defined value
}
