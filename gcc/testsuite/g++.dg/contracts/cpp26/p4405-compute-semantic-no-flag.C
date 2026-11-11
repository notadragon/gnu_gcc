// P4405: without -fcontracts-p4405, prove and diagnose are not in any
// contract's allowed set, so a compute_semantic facet returning one is an
// error.
//
// Mirror: clang/test/Contracts/p4405-compute-semantic-no-flag.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

template<evaluation_semantic R>
struct to {
  using assertion_control_object = to;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return R; }
};

constexpr to<evaluation_semantic::prove> to_prove{};
constexpr to<evaluation_semantic::diagnose> to_diagnose{};

int a (int x) pre<to_prove> (x > 0) { return x; }	// { dg-error "not in the allowed evaluation semantics" }
int b (int x) pre<to_diagnose> (x > 0) { return x; }	// { dg-error "not in the allowed evaluation semantics" }
