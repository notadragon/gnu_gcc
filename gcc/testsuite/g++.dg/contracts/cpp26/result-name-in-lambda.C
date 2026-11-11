// A lambda in a postcondition may use the result name, by implicit or
// explicit capture (a result binding is a local entity), including in
// constant expressions.
//
// Mirror: clang/test/Contracts/constexpr-result-name-in-lambda.cpp and
// result-name-explicit-capture.cpp in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

constexpr int fl (const int x) post (r : [&] { return r == x; } ()) { return x; }
static_assert (fl (4) == 4);
constexpr int fc (const int x) post (r : [r] { return r > 0; } ()) { return x; }
static_assert (fc (4) == 4);
