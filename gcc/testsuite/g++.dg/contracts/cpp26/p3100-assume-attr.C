// P3100: the [[assume]] attribute is a configurable implicit contract assertion
// (group ub:dcl.attr.assume.false -- the assumed condition being false is that
// core-language UB).  For a side-effect-free predicate every semantic is
// available.  Here: observe reports (assertion_kind::implicit) then continues;
// ignore drops the assumption (no check, no hint); the default is assume (no
// runtime check).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-assume-attr.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace cs = std::contracts;
static int calls = 0;
static bool all_implicit = true;
static const char *last_comment = nullptr;
void handle_contract_violation (const cs::contract_violation &v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit)
    all_implicit = false;
  last_comment = v.comment ();
}

namespace obs_ns { int f (int x) { [[assume (x > 0)]]; return x; } }   // observe
namespace ign_ns { int f (int x) { [[assume (x > 0)]]; return x; } }   // ignore
int def_f (int x) { [[assume (x > 0)]]; return x; }                    // assume

int main () {
  // observe: predicate false -> handler runs, then continues (returns x).
  if (obs_ns::f (-1) != -1) __builtin_abort ();
  if (calls != 1 || !all_implicit) __builtin_abort ();
  if (__builtin_strcmp (last_comment, "assumed condition is false") != 0)
    __builtin_abort ();

  // observe: predicate true -> no handler.
  if (obs_ns::f (5) != 5) __builtin_abort ();
  if (calls != 1) __builtin_abort ();

  // ignore: dropped, no check even when the predicate is false.
  if (ign_ns::f (-1) != -1) __builtin_abort ();
  if (calls != 1) __builtin_abort ();

  // assume (builtin default): no runtime check (call with a true predicate).
  if (def_f (5) != 5) __builtin_abort ();
  if (calls != 1) __builtin_abort ();
}
