// P3100: signed integer overflow (ub:expr.expr.eval.signed.integer) for +, -, *.
// ignore -> the defined 2's-complement wrapped result, no handler.
// observe -> the throwing observe semantic is not representable in the
// middle-end, so it is excluded from this check's allowed set; with
// -fcontracts-p4298 the best-fit at the same safety level is noexcept_observe,
// whose nothrow handler runs (assertion_kind::implicit) and then continues with
// the wrapped result.  (This is the first check where "ignore" is not a no-op:
// it must make overflow defined, which also disables the optimizer's
// no-overflow assumption -- see p3100-overflow-codegen.C.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-overflow.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <climits>

namespace cs = std::contracts;

static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation (const cs::contract_violation& v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit)
    all_implicit = false;
}

namespace ign_ns {                    // ignore
  int add (int a, int b) { return a + b; }
  int mul (int a, int b) { return a * b; }
}
namespace obs_ns {                    // observe
  int add (int a, int b) { return a + b; }
}

int main () {
  // ignore: defined wrapped result, no handler.
  if (ign_ns::add (INT_MAX, 1) != INT_MIN) __builtin_abort ();
  if (ign_ns::mul (INT_MAX, 2) != -2) __builtin_abort ();
  if (calls != 0) __builtin_abort ();

  // observe: handler runs, continues with the wrapped result.
  if (obs_ns::add (INT_MAX, 1) != INT_MIN) __builtin_abort ();
  if (calls != 1) __builtin_abort ();
  if (!all_implicit) __builtin_abort ();

  // no overflow: normal result, no handler either way.
  if (ign_ns::add (2, 3) != 5) __builtin_abort ();
  if (obs_ns::add (2, 3) != 5) __builtin_abort ();
  if (calls != 1) __builtin_abort ();
}
