// P3400: a label's allowed_semantics must be able to permit the P4298 noexcept
// variants.  An allowed-set probe that stops at CES_ASSUME, over a
// base mask excluding the noexcept bits, resolves a label allowing only
// noexcept_enforce to the empty set and rejects it with
// "assertion-control label allows no evaluation semantics".  The
// noexcept variants are therefore probed too, so the label permits
// noexcept_enforce under -fcontracts-p4298 and this compiles cleanly.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p4298 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct ne_only_t
{
  using assertion_control_object = ne_only_t;
  static constexpr evaluation_semantic_set allowed_semantics
    = { evaluation_semantic::noexcept_enforce };
  constexpr evaluation_semantic
  compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::noexcept_enforce; }
};
constexpr ne_only_t ne_only{};

// The label permits only noexcept_enforce and compute_semantic selects it; this
// must be accepted (no "allows no evaluation semantics" error).
void f (int x) pre<ne_only> (x > 0) { }

int main () { return 0; }
