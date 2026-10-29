// P3400: an allowed_semantics facet is read as evaluation_semantic_set
// ({label.allowed_semantics}), as in the concept, so a label holding a single
// evaluation_semantic restricts the semantic to that one.
//
// Clang: clang/test/Contracts/p3400-scalar-allowed-semantics.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

struct only_observe_t {
  using assertion_control_object = only_observe_t;
  static constexpr evaluation_semantic allowed_semantics
    = evaluation_semantic::observe;
};
constexpr only_observe_t only_observe{};
static_assert (std::contracts::labels::allowed_semantics_label<only_observe_t>);

static int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{ ++violations; }

void f (int x) pre<only_observe> (x > 0) {}

int main () {
  f (-1);                           // clamped to observe: continues
  if (violations != 1)
    __builtin_abort ();
}
