// P3595: the entry -fcontracts-client-check=all stands for comes after the
// configuration files, so a file's caller-side entries are consulted first
// and a query no file entry matches falls through to it.  The file ignores
// caller-side preconditions of functions in namespace n and every
// caller-side postcondition; ::f's precondition matches no file entry and
// is checked with -fcontract-evaluation-semantic='s semantic.
//
// The definition side is turned off, so a violation here can only be
// reported by a caller-side check.
// Mirror: none (Clang has no caller-side checking).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -fcontracts-client-check=all -fcontracts-definition-check=off" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-client-check-config-first.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

int violations = 0;

void
handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.semantic () != evaluation_semantic::observe)
    __builtin_abort ();
  ++violations;
}

namespace n {
int f (int a) pre (a > 0) { return a; }
}

int f (int a) pre (a > 0) { return a; }
int g (int a) post (r : r > 0) { return a; }

int
main ()
{
  n::f (-1);
  f (-1);
  g (-1);
  if (violations != 1)
    __builtin_abort ();
  return 0;
}
