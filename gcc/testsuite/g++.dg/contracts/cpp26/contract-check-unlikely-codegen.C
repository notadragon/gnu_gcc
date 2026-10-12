// The violation arm of an explicit contract check is marked unlikely, as
// [[unlikely]] would mark it, under every semantic that checks
// (notadragon_wg21 DECISIONS.md E63 (a)): one cold-label prediction per
// check, none for a check that is not emitted.
//
// Mirror: clang/test/Contracts/contract-check-unlikely-codegen.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4298 -fcontract-evaluation-semantic=observe -fdump-tree-original" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace L = std::contracts::labels;
using enum std::contracts::evaluation_semantic;

constexpr L::allowed_semantics_t<noexcept_observe> nx_observe{};
constexpr L::allowed_semantics_t<noexcept_enforce> nx_enforce{};

int f_obs (int x) pre (x > 0) post (r : r > 0)
{
  contract_assert (x != 3);
  return x;
}
void f_enf (int x) pre<L::always_enforce> (x > 0) { }
void f_quick (int x) pre<L::always_quick_enforce> (x > 0) { }
void f_nxo (int x) pre<nx_observe> (x > 0) { }
void f_nxe (int x) pre<nx_enforce> (x > 0) { }
void f_ign (int x) pre<L::symbolic> (x > 0) { }

// { dg-final { scan-tree-dump-times "predicted unlikely by cold label predictor" 7 "original" } }
