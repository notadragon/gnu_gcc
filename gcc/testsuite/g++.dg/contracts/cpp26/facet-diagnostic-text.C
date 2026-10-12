// The two P3400 label errors (no allowed semantics; a compute_semantic
// result outside them) have diagnostics of their own.
//
// Mirror: clang/test/Contracts/facet-errors-use-wrong-diagnostic.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

struct bad_t {
  using assertion_control_object = bad_t;
  static constexpr std::contracts::evaluation_semantic_set allowed_semantics
    {evaluation_semantic::observe};
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::enforce; }
};
constexpr bad_t bad{};

void f (int x) pre<bad> (x > 0) {} // { dg-error "compute_semantic. result is not in the allowed evaluation semantics" }
