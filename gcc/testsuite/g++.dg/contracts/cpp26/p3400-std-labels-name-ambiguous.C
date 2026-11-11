// P3400: a label is looked up in std::contracts::labels too, and <contracts>
// declares the semantic restriction labels there (always_observe,
// terminating, ...), so a user label of the same name is ambiguous -- at
// namespace scope, and in a namespace named by "using contract_control
// namespace" -- while a qualified name is not.  Mirrored from the EDG front
// end (edgcpp_compiler fork, M34-EDG).
//
// Mirror: tests/tests/contracts/p3400/labels_std_name_ambiguous.sft.cpp in
// the edgcpp_compiler fork;
// clang/test/Contracts/p3400-std-labels-name-ambiguous.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

struct fixed_t
{
  using assertion_control_object = fixed_t;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  {
    return evaluation_semantic::observe;
  }
};

constexpr fixed_t always_observe{};

int f (int x) pre<always_observe> (x > 0) { return x; }	// { dg-error "reference to .always_observe. is ambiguous" }
int g (int x) pre<::always_observe> (x > 0) { return x; }	// OK
int h (int x) pre<std::contracts::labels::always_observe> (x > 0) { return x; }	// OK

namespace ns { constexpr fixed_t terminating{}; }
using contract_control namespace ns;

int k (int x) pre<terminating> (x > 0) { return x; }	// { dg-error "reference to .terminating. is ambiguous" }
int m (int x) pre<ns::terminating> (x > 0) { return x; }	// OK
