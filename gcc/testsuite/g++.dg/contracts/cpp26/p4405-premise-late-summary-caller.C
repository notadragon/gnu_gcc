// P4405 premises decided by this TU (T1-GCC): as in
// p4405-premise-late-summary.C, but the replaceable function f's only
// unsettled contract evaluation is a caller-side diagnose evaluation of
// cs::k's precondition, deferred to the shadow, which proves it soundly from
// h's enforced precondition: the shadow inlines the static h, whose copy is
// this TU's, while f calls it out of line (the parameters provoke that, as in
// p4405-premise-shadow-only.C, with an assume evaluation that a label
// settles, so that it does not make f a refinable body).  The assumption made
// of the proof tells f's late value-range pass that the result is positive;
// keeping it makes f a refinable body, so use_f, compiled later, keeps its
// test of the result, since the definition the program calls may be another
// TU's, which assumes nothing.
//
// Mirror: clang/test/Contracts/p4405-premise-late-summary-caller.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "--param large-function-insns=40 --param large-function-growth=10" }
// { dg-additional-options "--param max-inline-insns-single=200 --param max-inline-insns-auto=200 -fno-partial-inlining" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-late-summary-caller.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template<evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};
constexpr allow<assume> assume_only{};

int bad ();
int sink (int);
int big (int);

namespace cs {
void k (int x) pre (x > 0);
}

static int h (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

[[gnu::noinline]] inline int f (int x, int y)
{
  contract_assert <assume_only> (x > 0 && x < 1000);
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  h (y);
  cs::k (y);	// sound
  return y;
}

int use_f (int x, int y)
{
  int r = f (x, y);
  if (r <= 0)
    return bad ();
  return r;
}

// { dg-final { scan-tree-dump {\(line 61\): sound} "contract_shadow" } }
// The real f calls h.
// { dg-final { scan-tree-dump {h[.a-z0-9]* \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump-times {contract obligation [0-9]+: assumed} 1 "contract_proven" } }
// f is a refinable body, and use_f still calls bad.
// { dg-final { scan-tree-dump {refinable body[^\n]*\n[^\n]*int f \(int x, int y\)} "optimized" } }
// { dg-final { scan-tree-dump-times {bad \(\)} 1 "optimized" } }
