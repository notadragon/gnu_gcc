// P3595: caller-side dynamic selection with no selector of the program's
// own.  The weak default returns the configured semantic, and the
// caller-side dispatch adjusts it to the allowed set and passes it through
// the label's compute_semantic once, as callee-side
// (p3595-dynamic-weak-default-compute.C; GCC-906 applied compute_semantic
// twice).  The configuration selects enforce caller-side; compute_semantic
// maps enforce to observe and observe to ignore, so the call is checked
// with observe: the handler is called once and the program continues.  The
// callee side is ignored (-fcontract-evaluation-semantic=ignore, which
// compute_semantic leaves alone).
// Mirror: none (Clang has no caller-side checking).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-dyn-weak-compute.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>

using std::contracts::evaluation_semantic;

struct chain_t {
  using assertion_control_object = chain_t;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic s) const
  {
    return (s == evaluation_semantic::enforce ? evaluation_semantic::observe
	    : s == evaluation_semantic::observe ? evaluation_semantic::ignore
	    : s);
  }
};
constexpr chain_t chain{};

static int handled;
static evaluation_semantic seen;

void
handle_contract_violation (const std::contracts::contract_violation &v)
{
  ++handled;
  seen = v.semantic ();
}

int f (int x) pre<chain> (x > 0) { return x; }
int call () { return f (-1); }

int
main ()
{
  call ();
  if (handled != 1 || seen != evaluation_semantic::observe)
    __builtin_abort ();
}
