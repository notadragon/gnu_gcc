// P3595: several match criteria in a single "match" object are AND-ed together
// -- an entry applies only when *every* criterion it names matches.  Existing
// tests exercise each criterion (kind, namespace, location, group, caller) in
// isolation; this one pins the combining behaviour: the first entry matches
// only a callee-side precondition in namespace "app", so a matching kind in the
// wrong namespace and a matching namespace with the wrong kind both fall
// through to the catch-all.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-combined-match.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
//
// Config:
//   1. namespace "app" AND kind "pre" AND callee-side -> observe
//   2. everything else                                -> ignore

#include <contracts>

static int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation &) {
  ++violations;
}

namespace app {
  // pre in "app": matches all three criteria -> observe (handler runs).
  void f_pre (int x) pre (x > 0) { }
  // post in "app": kind mismatch -> catch-all ignore (no handler).
  int f_post (int x) post (r: r > 0) { return x; }
}

// pre in the global namespace: namespace mismatch -> catch-all ignore.
void g_pre (int x) pre (x > 0) { }

int main () {
  app::f_pre (-1);            // observe -> handler called once
  if (violations != 1) __builtin_abort ();

  app::f_post (-1);           // ignore  -> no handler
  if (violations != 1) __builtin_abort ();

  g_pre (-1);                 // ignore  -> no handler
  if (violations != 1) __builtin_abort ();

  // Non-violating calls stay quiet.
  app::f_pre (1);
  (void) app::f_post (1);
  g_pre (1);
  if (violations != 1) __builtin_abort ();
}
