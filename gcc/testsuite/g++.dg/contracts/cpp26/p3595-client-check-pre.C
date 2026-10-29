// P3595: -fcontracts-client-check=pre with no configuration file checks
// preconditions at the call, with the semantic of
// -fcontract-evaluation-semantic=, and not postconditions: it is the
// configuration entry
//   {"match": {"caller": true, "kind": "pre"},
//    "output": {"semantic": "<semantic>"}}
// (GCC-912: no caller-side check was selected).
//
// The definition side is turned off, so a violation here can only be
// reported by a caller-side check.
// Mirror: none (Clang has no caller-side checking).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -fcontracts-client-check=pre -fcontracts-definition-check=off" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::assertion_kind;
using std::contracts::evaluation_semantic;

int pre_violations = 0;
int post_violations = 0;

void
handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.semantic () != evaluation_semantic::observe)
    __builtin_abort ();
  if (v.kind () == assertion_kind::pre)
    ++pre_violations;
  else if (v.kind () == assertion_kind::post)
    ++post_violations;
}

int f (int a) pre (a > 0) { return a; }
int g (int a) post (r : r > 0) { return a; }

int
main ()
{
  f (-1);
  g (-1);
  if (pre_violations != 1 || post_violations != 0)
    __builtin_abort ();
  return 0;
}
