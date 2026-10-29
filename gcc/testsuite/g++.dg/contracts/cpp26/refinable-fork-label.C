// The refinable-body rule with P3400 labels: an assertion whose label's
// allowed_semantics facet (which every translation unit sees in the
// declaration) settles its evaluation in both directions does not make its
// function a refinable body; any other does (contract_evaluation_settled_p).
//
// refinable-fork-label-u.cc holds the callers and the definitions they are
// compiled with; refinable-fork-label-k.cc, linked first, holds the
// definitions the linker keeps (refinable-fork-label.json gives each
// assertion its semantic by location).
//
// u_open: {enforce, observe}: the callers' definition enforces, the kept one
// observes and continues; use_u_open (-5) must be 0.
// u_mixed: {enforce, quick_enforce}: the callers' definition traps and
// cannot throw, the kept one enforces with a handler that throws;
// use_u_mixed (-5) must catch it.
// s_*: settled -- {quick_enforce, noexcept_enforce, assume}, {enforce,
// quick_enforce} on a noexcept function, {enforce} alone: whichever
// definition runs, a call that returns returns the same, and the callers
// may use their definition's facts (refinable-body-label.C shows that they
// do); valid arguments only, since a violation ends the program.
//
// -fno-ipa-sra: otherwise the callers call a local clone of their own
// translation unit's definition, which hides the problem.
//
// Mirror: clang/test/Contracts/Runnable/refinable-fork-label.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4298 -fcontracts-allow-assume -O2 -fno-ipa-sra" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-fork-label.json" }
// { dg-additional-sources "refinable-fork-label-k.cc refinable-fork-label-u.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int handled;
bool throwing;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
  if (throwing)
    throw 42;
}

int use_u_open (int);
int use_u_mixed (int);
int use_s_settled (int);
int use_s_mixed_nx (int);
int use_s_enforce_only (int);

int
main ()
{
  if (use_u_open (-5) != 0 || handled != 1)
    __builtin_abort ();
  throwing = true;
  if (use_u_mixed (-5) != -1 || handled != 2)
    __builtin_abort ();
  if (use_s_settled (5) != 1
      || use_s_mixed_nx (5) != 1
      || use_s_enforce_only (5) != 1
      || handled != 2)
    __builtin_abort ();
}
