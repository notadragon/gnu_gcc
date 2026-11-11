// A postcondition naming the result of a reference-returning constexpr
// function: the valid calls are constant, the violated one is diagnosed.
//
// Mirror: clang/test/Contracts/constexpr-post-reference-result.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

constexpr const int &pick (const int &a) post (r : r > 0) { return a; } // { dg-error "contract predicate is false in constant expression" }
constexpr int g = 5;
constexpr int v = pick (g); // OK
static_assert (v == 5);
constexpr int z = 0;
constexpr int bad = pick (z);

constexpr int gx = 5;
constexpr decltype(auto) pick2 () post (r : r > 0) { return (gx); }
constexpr int w = pick2 (); // OK
