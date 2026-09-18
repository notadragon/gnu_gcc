// P3595: mixed pre+post caller-location regression test.
// One callee has BOTH a precondition and a postcondition, so building its
// wrapper exercises copy_and_remap_contracts's pre/post skip-continue path
// (cmk_pre when only the precondition is active, cmk_all when both are).
// Two call sites in different namespaces resolve to different caller-side
// semantics via namespace-based caller filtering: "hot" -> observe, "cold"
// -> ignore.  Both the precondition and the postcondition are violated at
// each call site.  At the "hot" call site both checks must fire; at the
// "cold" call site neither must fire.  If compute_caller_semantic_tuple's
// postcondition tuple element were misaligned (e.g. reading the
// precondition's resolved semantic instead of the postcondition's), the
// counts below would not match.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-location-prepost.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int pre_fired = 0;
static int post_fired = 0;

void handle_contract_violation (const std::contracts::contract_violation &v) {
  if (v.kind () == std::contracts::assertion_kind::pre)
    ++pre_fired;
  else if (v.kind () == std::contracts::assertion_kind::post)
    ++post_fired;
  else
    __builtin_abort ();
}

int g (int x) pre(x > 0) post(r: r > 0) { return x; }

namespace hot {
  int call () { return g (-1); }
}

namespace cold {
  int call () { return g (-1); }
}

int main () {
  hot::call ();
  // Active caller-side semantics (observe): both the pre and post checks
  // fire exactly once each.
  if (pre_fired != 1 || post_fired != 1) __builtin_abort ();

  cold::call ();
  // Ignored caller-side semantics: neither check fires again.
  if (pre_fired != 1 || post_fired != 1) __builtin_abort ();

  return 0;
}
