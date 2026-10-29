// The refinable-body rule for every evaluation semantic: a function another
// translation unit may also define (here an inline function) whose body
// holds a contract assertion is a refinable body, so calls that do not
// inline it use no facts derived from this translation unit's definition --
// another definition may evaluate the assertion with another semantic, and
// the linker may keep that one.
//
// refinable-fork-sem-u.cc holds the callers and the definitions they are
// compiled with; refinable-fork-sem-k.cc, linked first, holds the
// definitions the linker keeps (refinable-fork-sem.json gives each
// assertion its semantic by location).
//
// a_*: the callers' definition evaluates the assertion with a semantic that
// does not continue past a violation (enforce, noexcept_enforce,
// quick_enforce -- an inline trap -- and assume), so its return value is
// positive, but the kept definition ignores or observes the assertion:
// use_a_* (-5) must be 0.
//
// b_*: the callers' definition cannot throw (ignore, quick_enforce,
// noexcept_enforce, noexcept_observe, assume, and a dynamic selection
// (P3595) that selects ignore), but the kept definition enforces the
// assertion and the violation handler throws: use_b_* (-5) must catch it.
//
// -fno-ipa-sra: otherwise the callers call a local clone of their own
// translation unit's definition, which hides the problem.
//
// Mirror: clang/test/Contracts/Runnable/refinable-fork-sem.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4298 -fcontracts-allow-assume -O2 -fno-ipa-sra" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-fork-sem.json" }
// { dg-additional-sources "refinable-fork-sem-k.cc refinable-fork-sem-u.cc" }
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

int use_a_enforce (int);
int use_a_nx_enforce (int);
int use_a_quick (int);
int use_a_assume (int);
int use_a_vs_observe (int);
int use_b_ignore (int);
int use_b_quick (int);
int use_b_nx_enforce (int);
int use_b_nx_observe (int);
int use_b_assume (int);
int use_b_dyn (int);

int
main ()
{
  if (use_a_enforce (-5) != 0
      || use_a_nx_enforce (-5) != 0
      || use_a_quick (-5) != 0
      || use_a_assume (-5) != 0
      || handled != 0)
    __builtin_abort ();
  if (use_a_vs_observe (-5) != 0 || handled != 1)
    __builtin_abort ();

  throwing = true;
  int (*const uses[]) (int) = { use_b_ignore, use_b_quick, use_b_nx_enforce,
				 use_b_nx_observe, use_b_assume, use_b_dyn };
  for (auto use : uses)
    {
      int h = handled;
      if (use (-5) != -1 || handled != h + 1)
	__builtin_abort ();
    }
}
