// In the P3400 wording an assertion-control-expression is a
// postfix-expression alternative ([expr.post.general]), so member access may
// follow it directly.
//
// Mirror: clang/test/Contracts/contract-control-postfix.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }

struct L { using assertion_control_object = L; int v; };
constexpr L make () { return L{3}; }

constexpr int q = contract_control (make ()).v;  // OK, a postfix-expression
static_assert (q == 3);
