// P3100 x P4298: signed overflow configured to noexcept_observe.  The nothrow
// handler runs with a populated contract_violation (kind=implicit, the right
// semantic, comment), then execution continues with the defined wrapped result.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-overflow-noexcept.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <climits>
#include <cstring>

namespace cs = std::contracts;

static int calls = 0;
void handle_contract_violation (const cs::contract_violation& v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit) __builtin_abort ();
  if (v.semantic () != cs::evaluation_semantic::noexcept_observe) __builtin_abort ();
  if (std::strcmp (v.comment (), "signed integer overflow") != 0) __builtin_abort ();
}

__attribute__((noinline)) int add (int a, int b) { return a + b; }

int main () {
  int r = add (INT_MAX, 1);   // handler runs (nothrow), then continues
  if (calls != 1) __builtin_abort ();
  if (r != INT_MIN) __builtin_abort ();   // continued with the wrapped result
}
