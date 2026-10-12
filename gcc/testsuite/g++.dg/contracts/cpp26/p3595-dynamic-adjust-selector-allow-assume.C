// With -fcontracts-allow-assume, a P3595 dynamic selector that returns
// assume makes an allowed choice, which takes the ignore path: no check, the
// predicate is not evaluated (notadragon_wg21 DECISIONS.md E66).  A label
// that does not allow assume still adjusts it, here upward to observe.
// p3595-dynamic-adjust-selector.C is the form without the flag.
//
// Mirror: clang/test/Contracts/p3595-dynamic-adjust-selector-allow-assume.cpp
// in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p3098 -fcontracts-allow-assume" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-adjust-selector.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

extern "C" evaluation_semantic adjust_sel ()
{ return evaluation_semantic::assume; }

static int evaluated, observed, inits;

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.semantic () == evaluation_semantic::observe)
    ++observed;
  else
    __builtin_abort ();
}

struct observe_or_enforce_t {
  using assertion_control_object = observe_or_enforce_t;
  static constexpr evaluation_semantic_set allowed_semantics
    = {evaluation_semantic::observe, evaluation_semantic::enforce};
};
constexpr observe_or_enforce_t observe_or_enforce{};

struct G {
  G () { ++inits; }
  G (const G &) { ++inits; }
};

bool chk (int x) { ++evaluated; return x > 0; }

void f (int x) pre (chk (x)) { }
void lf (int x) pre<observe_or_enforce> (chk (x)) { }
int pf (const int x) post [g = G ()] (chk (x)) { return x; }

int main ()
{
  f (-1);
  pf (-1);
  if (evaluated != 0 || observed != 0 || inits != 0)
    __builtin_abort ();		// assume: the ignore path
  lf (-1);
  if (evaluated != 1 || observed != 1 || inits != 0)
    __builtin_abort ();		// not allowed by the label: observe
}
