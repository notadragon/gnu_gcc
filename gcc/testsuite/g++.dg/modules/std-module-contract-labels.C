// GCC-684: "import std;" can name every std::contracts::labels name that
// <contracts> declares, including the concept assertion_control_object,
// which std.cc did not export.  Like std-module-contract-names.C, it lives
// here because --compile-std-module needs this directory's dg-module-cmi
// harness.
//
// Mirror: clang/test/Contracts/std-module-contracts-labels.cpp in the
// llvm_llvm-project fork.

// { dg-additional-options "-fmodules --compile-std-module -fcontracts -fcontracts-p3400" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi std }
// { dg-module-cmi std.compat }
// { dg-module-cmi <bits/stdc++.h> }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import std;

namespace L = std::contracts::labels;

constexpr L::empty_label_t e = L::empty_label;
static_assert (L::assertion_control_object<L::empty_label_t>);
static_assert (L::assertion_control_object<L::review_t>);
static_assert (L::assertion_control_object<L::fixed_message_label_t>);
static_assert (!L::assertion_control_object<int>);

template <typename T> concept all_named
  = (L::semantic_computation_label<T> || L::allowed_semantics_label<T>
     || L::local_violation_label<T> || L::compute_comment_label<T>
     || L::compute_message_label<T> || L::queryable_label<T>
     || L::identification_label<T> || true);
static_assert (all_named<int>);

constexpr auto g = L::assertion_group_label ("a");
using namespace std::contracts::labels;
constexpr auto g2 = "b"group;
constexpr auto c = L::review | L::empty_label;
