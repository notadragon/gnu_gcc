// P3595: a "dynamic" entry with no "semantic" is deliberately accepted by
// the config parser -- provideweak is forced false, no weak definition is
// emitted, and the user supplies the selector themselves.
//
// Regression test: contract_config_resolve has to clamp entry->semantic for
// this shape too.  Left at CES_INVALID, the
// definition-side ensure_evaluation_semantic treats it as a hard error
// ("no valid evaluation semantic for contract assertion") while the
// caller-side resolve_caller_semantic quietly turns the identical
// CES_INVALID into CES_IGNORE and works.  That makes the whole "provide your
// own strong selector, no weak fallback" pattern unusable on the
// definition side, with no workaround: first-match-wins means a
// trailing catch-all entry never gets consulted.
//
// Nothing is under-enforced by treating it as ignore: with a dynamic
// descriptor present the contract stays active regardless of the
// compile-time default (contract_active_p), and the cached semantic is
// only ever the value a weak definition would return -- which is not
// emitted here.
//
// The other test of this area pairs "provideweak": false with an explicit
// "semantic" (p3595-dynamic-noweak.C), so only this file covers the shape.

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-nodefault.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

int violations = 0;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

// The only definition of the selector: provideweak is false and no
// "semantic" was given, so the compiler emits nothing for it.
evaluation_semantic
p3595_sel_nd ()
{
  return evaluation_semantic::observe;
}

// Definition-side contract: this is the side that hard-errors on CES_INVALID.
void
f (const int x) pre (x > 0)
{
}

int
main ()
{
  f (-1);
  if (violations != 1)
    __builtin_abort ();
  return 0;
}
