// "import std;" names every part of the P3400 library surface <contracts>
// declares: the evaluation_semantic_set members, the classification
// functions, allowed_semantics_t, the semantic restriction labels and the
// is_C variable templates, and D4405's analysis_semantic_label.  Like
// std-module-contract-labels.C, it lives here because --compile-std-module
// needs this directory's dg-module-cmi harness.
//
// Mirror: clang/test/Contracts/std-module-contracts-p3400-surface.cpp in the
// llvm_llvm-project fork.

// { dg-additional-options "-fmodules --compile-std-module -fcontracts -fcontracts-p3400" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi std }
// { dg-module-cmi std.compat }
// { dg-module-cmi <bits/stdc++.h> }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import std;

namespace C = std::contracts;
namespace L = std::contracts::labels;
using enum C::evaluation_semantic;

constexpr bool
set_ops ()
{
  C::evaluation_semantic_set s;
  s.insert (observe);
  s.insert ({enforce, ignore});
  s.erase (ignore);
  s |= C::evaluation_semantic_set {quick_enforce};
  s &= ~C::evaluation_semantic_set {quick_enforce};
  bool ok = s.size () == 2;
  s.clear ();
  return ok && s.empty ();
}
static_assert (set_ops ());

static_assert (C::is_checked (observe) && C::is_unchecked (ignore));
static_assert (C::is_terminating (enforce) && C::is_continuing (observe));
static_assert (!C::is_terminating (assume) && !C::is_continuing (assume));

static_assert (L::allowed_semantics_t<observe>::allowed_semantics
	       .contains (observe));
static_assert (L::terminating.allowed_semantics.contains (enforce));
static_assert (L::symbolic.allowed_semantics.contains (ignore));
static_assert (L::always_enforce.allowed_semantics.contains (enforce));
static_assert (L::always_quick_enforce.allowed_semantics
	       .contains (quick_enforce));
static_assert (L::always_observe.allowed_semantics.contains (observe));
static_assert (L::always_ignore.allowed_semantics.contains (ignore));
static_assert (!L::never_enforce.allowed_semantics.contains (enforce));
static_assert (!L::never_quick_enforce.allowed_semantics
	       .contains (quick_enforce));
static_assert (!L::never_observe.allowed_semantics.contains (observe));
static_assert (!L::never_ignore.allowed_semantics.contains (ignore));

static_assert (L::is_assertion_control_object<L::review>);
static_assert (L::is_semantic_computation_label<L::review>);
static_assert (L::is_allowed_semantics_label<L::terminating>);
static_assert (!L::is_local_violation_label<L::review>);
static_assert (!L::is_compute_comment_label<L::review>);
static_assert (!L::is_compute_message_label<L::review>);
static_assert (!L::is_queryable_label<L::review>);
static_assert (!L::is_identification_label<L::review>);
// D4405's analysis_semantic_label, with the labels terminating and symbolic
// now allowing prove.
static_assert (!L::analysis_semantic_label<decltype (L::terminating)>);
static_assert (L::terminating.allowed_semantics.contains (prove));
static_assert (L::symbolic.allowed_semantics.contains (prove));
