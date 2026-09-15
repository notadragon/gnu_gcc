// P3100: whether the checking semantics are available for a configured
// [[assume]] depends on the PREDICATE.  A side-effect-free predicate -- here a
// call to a [[gnu::pure]] function -- can be evaluated, so observe checks it.
// An opaque function call may have side effects, so it is NOT checkable: the
// configured checking semantic clamps to ignore and the predicate is never
// evaluated (no side effect, no handler).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-assume-attr-pure.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace cs = std::contracts;
static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation (const cs::contract_violation &v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit)
    all_implicit = false;
}

[[gnu::pure]] bool pure_pred (int x);      // pure -> checkable
bool opaque_pred (int x);                  // opaque -> not checkable
static int opaque_side = 0;
bool opaque_pred (int x) { ++opaque_side; return x > 0; }   // has a side effect
[[gnu::pure]] bool pure_pred (int x) { return x > 0; }

namespace obs_pure   { int f (int x) { [[assume (pure_pred (x))]];   return x; } }
namespace obs_opaque { int f (int x) { [[assume (opaque_pred (x))]]; return x; } }

int main () {
  // pure predicate + observe: CHECKED -> false fires the handler.
  obs_pure::f (-1);
  if (calls != 1 || !all_implicit) __builtin_abort ();

  // opaque predicate + observe: clamps to ignore -> no check, and the predicate
  // is not evaluated (no side effect).
  obs_opaque::f (-1);
  if (calls != 1) __builtin_abort ();
  if (opaque_side != 0) __builtin_abort ();
}
