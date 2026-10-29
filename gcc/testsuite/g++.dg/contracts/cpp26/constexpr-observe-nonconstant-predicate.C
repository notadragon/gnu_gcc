// Under observe, a predicate that is not a core constant expression in a
// manifestly constant-evaluated context is a contract violation -- a
// diagnostic, and evaluation continues: each initializer below stays
// constant, with a warning.
//
// Mirror: clang/test/Contracts/constexpr-observe-nonconstant-predicate.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }

int g (int x) { return x; } // not constexpr
constexpr int f (int x) pre (g (x) > 0) { return x; } // { dg-warning "contract condition is not constant" }
constexpr int y = f (1);
static_assert (y == 1);
constexpr int h (int x) { contract_assert (g (x) > 0); return x; } // { dg-warning "contract condition is not constant" }
constexpr int z = h (1);
int g2 = f (1); // constant-initialization
