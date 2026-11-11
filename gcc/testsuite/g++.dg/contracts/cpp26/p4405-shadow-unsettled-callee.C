// D4405R0, "Previous Assertions and Undefined Behavior" (DECISIONS.md
// E214): an obligation's predicate that calls, out of line, a replaceable
// function whose code holds an unsettled contract evaluation makes the
// proof conditional -- not diagnosed, never assumed -- however free of side
// effects this translation unit's copy is: another translation unit's copy,
// the one the program may run, may evaluate it with observe, whose
// violation handler may have side effects.  Every evaluation here is
// ignore (unsettled) but the prove ones, and each prove predicate holds
// whatever its call returns.
//
// * replaceable: a noinline inline function holding an ignored
//   contract_assert: conditional, not assumed.
// * internal: the same as a static function, an exact definition: judged
//   by its body (no check), sound and assumed.
// * plain: a noinline inline function with no contract evaluation: judged
//   by its body, sound and assumed.
// * via_internal: a static function calling the replaceable one out of
//   line: conditional, not assumed.
// * inlined: an always_inline function holding an ignored contract_assert,
//   inlined into the shadow: conditional, not assumed.
//
// A function with a contract assertion whose definition may be replaced
// is a refinable body, never found pure
// (p4405-own-pre-call-premise-sound.C); an exact definition cannot be
// replaced, so it is judged by its body.
//
// Mirror: clang/test/Contracts/p4405-shadow-unsettled-callee.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore -fcontracts-prove-assume" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
using namespace std::contracts::labels;

int sink (int);

[[gnu::noinline]] inline bool checked (int x)
{
  contract_assert (x != 5);
  return x > 0;
}

[[gnu::noinline]] static bool checked_internal (int x)
{
  contract_assert (x != 5);
  return x > 0;
}

[[gnu::noinline]] inline bool plain (int x) { return x > 0; }

[[gnu::noinline]] static bool via_checked (int x) { return checked (x); }

[[gnu::always_inline]] inline bool checked_inlined (int x)
{
  contract_assert (x != 5);
  return x > 0;
}

int replaceable (int x)
{
  if (x > 0)
    contract_assert<prove> (checked (x) || x > 0);	// { dg-bogus "cannot be proven" }
  return sink (x);
}

int internal (int x)
{
  if (x > 0)
    contract_assert<prove> (checked_internal (x) || x > 0);
  return sink (x);
}

int plain_callee (int x)
{
  if (x > 0)
    contract_assert<prove> (plain (x) || x > 0);
  return sink (x);
}

int via_internal (int x)
{
  if (x > 0)
    contract_assert<prove> (via_checked (x) || x > 0);	// { dg-bogus "cannot be proven" }
  return sink (x);
}

int inlined (int x)
{
  if (x > 0)
    contract_assert<prove> (checked_inlined (x) || x > 0);	// { dg-bogus "cannot be proven" }
  return sink (x);
}

// { dg-final { scan-tree-dump {contract obligation 0 \(line 65\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 1 \(line 72\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 2 \(line 79\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 3 \(line 86\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 4 \(line 93\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {contract obligation 1: assumed} "contract_proven" } }
// { dg-final { scan-tree-dump {contract obligation 2: assumed} "contract_proven" } }
// { dg-final { scan-tree-dump-times {: assumed} 2 "contract_proven" } }
