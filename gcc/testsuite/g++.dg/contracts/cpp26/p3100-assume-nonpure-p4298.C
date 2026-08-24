// P3100: an opaque (side-effecting / non-checkable) [[assume(cond)]] must never
// evaluate its predicate -- its allowed set is {assume, ignore}, so a configured
// checking semantic clamps to ignore (drop the assumption).  This must hold even
// under -fcontracts-p4298: the noexcept-variant widening of the allowed set must
// not re-admit a checking semantic that the predicate's own allowed set excludes.
// Regression: p4298 previously widened the mask ignoring the caller's allowed
// restriction, so the opaque predicate got evaluated (its side effect fired).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-assume-nonpure-p4298.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static volatile int called = 0;
// Not pure/const: a call with side effects -> opaque predicate.  The body uses
// only a store (no signed arithmetic) so the sole implicit assertion here is the
// [[assume]] itself.
__attribute__((noinline)) bool sideeffect () { called = 1; return true; }

void handle_contract_violation (const std::contracts::contract_violation &) { }

__attribute__((noinline)) void g () { [[assume (sideeffect ())]]; }

int main ()
{
  g ();
  // Opaque predicate must be dropped, not evaluated, even under p4298.
  if (called != 0)
    __builtin_abort ();
}
