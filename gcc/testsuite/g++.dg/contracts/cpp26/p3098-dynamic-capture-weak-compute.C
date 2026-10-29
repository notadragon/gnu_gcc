// A dynamically selected postcondition with captures chooses its semantic
// once, where the captures are initialized.  With no selector of the
// program's own, the weak default returns the configured semantic, which
// is adjusted to the allowed set and passed through the label's
// compute_semantic once (p3595-dynamic-weak-default-compute.C, whose
// configuration this test shares; GCC-906 applied compute_semantic twice).
// The configuration selects enforce; compute_semantic maps enforce to
// observe and observe to ignore, so the postcondition is checked with
// observe: the capture is initialized, the handler called once, and the
// program continues.
//
// Mirror: in the llvm_llvm-project fork,
// clang/test/Contracts/Runnable/p3098-dynamic-capture-weak-compute.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-weak-default-compute.json" }
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

static int inits, handled;
static evaluation_semantic seen;

struct G {
  G () { ++inits; }
  G (const G &) { ++inits; }
};

void
handle_contract_violation (const std::contracts::contract_violation &v)
{
  ++handled;
  seen = v.semantic ();
}

int f (int i) post<chain> [g = G ()] (false) { return i; }

int
main ()
{
  f (1);
  if (inits != 1 || handled != 1 || seen != evaluation_semantic::observe)
    __builtin_abort ();
}
